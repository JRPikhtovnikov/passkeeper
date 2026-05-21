#include "services/EntryService.h"

#include "crypto/CryptoProvider.h"

#include <stdexcept>
#include <utility>

namespace {

models::EncryptedBlob blobFromResult(const database::DatabaseManager::Result& result,
                                     int row,
                                     int nonce,
                                     int ciphertext,
                                     int tag)
{
    return {
        result.bytea(row, nonce),
        result.bytea(row, ciphertext),
        result.bytea(row, tag),
    };
}

QList<database::DatabaseManager::Param> blobParams(const models::EncryptedBlob& blob)
{
    return {
        database::DatabaseManager::Param::bytea(blob.nonce),
        database::DatabaseManager::Param::bytea(blob.ciphertext),
        database::DatabaseManager::Param::bytea(blob.tag),
    };
}

} // namespace

namespace services {

EntryService::EntryService(database::DatabaseManager& database, QByteArray encryptionKey)
    : m_database(database)
    , m_encryptionKey(std::move(encryptionKey))
{
    if (m_encryptionKey.size() != crypto::CryptoProvider::kAesKeySize) {
        throw std::invalid_argument("invalid entry encryption key");
    }
}

QList<models::PasswordEntry> EntryService::list(const QString& filter) const
{
    const auto result = m_database.exec(QStringLiteral(R"SQL(
SELECT e.id,
       COALESCE(e.category_id, -1),
       COALESCE(c.name, ''),
       e.title,
       e.username_nonce, e.username_ciphertext, e.username_tag,
       e.password_nonce, e.password_ciphertext, e.password_tag,
       e.url_nonce, e.url_ciphertext, e.url_tag,
       e.notes_nonce, e.notes_ciphertext, e.notes_tag,
       e.created_at,
       e.updated_at
FROM password_entries e
LEFT JOIN categories c ON c.id = e.category_id
ORDER BY lower(e.title)
)SQL"));

    const QString normalizedFilter = filter.trimmed().toCaseFolded();
    QList<models::PasswordEntry> entries;
    for (int row = 0; row < result.rows(); ++row) {
        const qint64 id = result.int64(row, 0);
        models::PasswordEntry entry;
        entry.id = id;
        entry.categoryId = result.int64(row, 1);
        entry.categoryName = result.value(row, 2);
        entry.title = result.value(row, 3);
        entry.username =
            decryptField(id, QStringLiteral("username"), blobFromResult(result, row, 4, 5, 6));
        entry.password =
            decryptField(id, QStringLiteral("password"), blobFromResult(result, row, 7, 8, 9));
        entry.url = decryptField(id, QStringLiteral("url"), blobFromResult(result, row, 10, 11, 12));
        entry.notes =
            decryptField(id, QStringLiteral("notes"), blobFromResult(result, row, 13, 14, 15));
        entry.createdAt = result.timestamp(row, 16);
        entry.updatedAt = result.timestamp(row, 17);

        if (matchesFilter(entry, normalizedFilter)) {
            entries.push_back(entry);
        }
    }
    return entries;
}

qint64 EntryService::create(const models::PasswordEntry& entry) const
{
    if (entry.title.trimmed().isEmpty()) {
        throw std::invalid_argument("entry title is required");
    }

    const auto idResult = m_database.exec(
        QStringLiteral("SELECT nextval(pg_get_serial_sequence('password_entries','id'))"));
    if (idResult.empty()) {
        throw std::runtime_error("entry id allocation failed");
    }
    const qint64 id = idResult.int64(0, 0);

    const auto username = encryptField(id, QStringLiteral("username"), entry.username);
    const auto password = encryptField(id, QStringLiteral("password"), entry.password);
    const auto url = encryptField(id, QStringLiteral("url"), entry.url);
    const auto notes = encryptField(id, QStringLiteral("notes"), entry.notes);

    QList<database::DatabaseManager::Param> params{
        database::DatabaseManager::Param::int64(id),
        database::DatabaseManager::Param::int64(entry.categoryId),
        database::DatabaseManager::Param::text(entry.title.trimmed()),
    };
    params.append(blobParams(username));
    params.append(blobParams(password));
    params.append(blobParams(url));
    params.append(blobParams(notes));

    m_database.execParams(QStringLiteral(R"SQL(
INSERT INTO password_entries(
    id, category_id, title,
    username_nonce, username_ciphertext, username_tag,
    password_nonce, password_ciphertext, password_tag,
    url_nonce, url_ciphertext, url_tag,
    notes_nonce, notes_ciphertext, notes_tag
) VALUES (
    $1, NULLIF($2, -1), $3,
    $4, $5, $6,
    $7, $8, $9,
    $10, $11, $12,
    $13, $14, $15
)
)SQL"),
                          params);
    return id;
}

void EntryService::update(const models::PasswordEntry& entry) const
{
    if (entry.id <= 0 || entry.title.trimmed().isEmpty()) {
        throw std::invalid_argument("valid entry id and title are required");
    }

    const auto username = encryptField(entry.id, QStringLiteral("username"), entry.username);
    const auto password = encryptField(entry.id, QStringLiteral("password"), entry.password);
    const auto url = encryptField(entry.id, QStringLiteral("url"), entry.url);
    const auto notes = encryptField(entry.id, QStringLiteral("notes"), entry.notes);

    QList<database::DatabaseManager::Param> params{
        database::DatabaseManager::Param::int64(entry.categoryId),
        database::DatabaseManager::Param::text(entry.title.trimmed()),
    };
    params.append(blobParams(username));
    params.append(blobParams(password));
    params.append(blobParams(url));
    params.append(blobParams(notes));
    params.append(database::DatabaseManager::Param::int64(entry.id));

    m_database.execParams(QStringLiteral(R"SQL(
UPDATE password_entries SET
    category_id = NULLIF($1, -1),
    title = $2,
    username_nonce = $3,
    username_ciphertext = $4,
    username_tag = $5,
    password_nonce = $6,
    password_ciphertext = $7,
    password_tag = $8,
    url_nonce = $9,
    url_ciphertext = $10,
    url_tag = $11,
    notes_nonce = $12,
    notes_ciphertext = $13,
    notes_tag = $14,
    updated_at = now()
WHERE id = $15
)SQL"),
                          params);
}

void EntryService::remove(qint64 id) const
{
    if (id <= 0) {
        throw std::invalid_argument("valid entry id is required");
    }

    m_database.execParams(QStringLiteral("DELETE FROM password_entries WHERE id = $1"),
                          {database::DatabaseManager::Param::int64(id)});
}

models::EncryptedBlob EntryService::encryptField(qint64 entryIdHint,
                                                 const QString& field,
                                                 const QString& value) const
{
    return crypto::CryptoProvider::encrypt(value.toUtf8(), m_encryptionKey, aad(entryIdHint, field));
}

QString EntryService::decryptField(qint64 entryId,
                                   const QString& field,
                                   const models::EncryptedBlob& blob) const
{
    return QString::fromUtf8(crypto::CryptoProvider::decrypt(blob, m_encryptionKey, aad(entryId, field)));
}

QByteArray EntryService::aad(qint64 entryId, const QString& field)
{
    return QStringLiteral("passkeeper-entry-v1:%1:%2")
        .arg(entryId)
        .arg(field)
        .toUtf8();
}

bool EntryService::matchesFilter(const models::PasswordEntry& entry, const QString& normalizedFilter)
{
    if (normalizedFilter.isEmpty()) {
        return true;
    }

    return entry.title.toCaseFolded().contains(normalizedFilter) ||
           entry.url.toCaseFolded().contains(normalizedFilter) ||
           entry.categoryName.toCaseFolded().contains(normalizedFilter);
}

} // namespace services
