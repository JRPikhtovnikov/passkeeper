#include "services/ExportImportService.h"

#include "crypto/CryptoProvider.h"

#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>

#include <stdexcept>

namespace {

constexpr int kExportIterations = 600000;

QJsonObject encodeBlob(const models::EncryptedBlob& blob)
{
    return {
        {QStringLiteral("nonce"), QString::fromLatin1(blob.nonce.toBase64())},
        {QStringLiteral("ciphertext"), QString::fromLatin1(blob.ciphertext.toBase64())},
        {QStringLiteral("tag"), QString::fromLatin1(blob.tag.toBase64())},
    };
}

models::EncryptedBlob decodeBlob(const QJsonObject& object)
{
    return {
        QByteArray::fromBase64(object.value(QStringLiteral("nonce")).toString().toLatin1()),
        QByteArray::fromBase64(object.value(QStringLiteral("ciphertext")).toString().toLatin1()),
        QByteArray::fromBase64(object.value(QStringLiteral("tag")).toString().toLatin1()),
    };
}

void writeFile(const QString& filePath, const QByteArray& content)
{
    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        throw std::runtime_error(QString("cannot open export file: %1").arg(file.errorString()).toStdString());
    }
    if (file.write(content) != content.size()) {
        throw std::runtime_error(QString("cannot write export file: %1").arg(file.errorString()).toStdString());
    }
}

QByteArray readFile(const QString& filePath)
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        throw std::runtime_error(QString("cannot open import file: %1").arg(file.errorString()).toStdString());
    }
    return file.readAll();
}

} // namespace

namespace services {

ExportImportService::ExportImportService(database::DatabaseManager& database,
                                         EntryService& entries,
                                         CategoryService& categories)
    : m_database(database)
    , m_entries(entries)
    , m_categories(categories)
{
}

void ExportImportService::exportEncrypted(const QString& filePath, const QString& exportPassword) const
{
    QJsonArray categoriesJson;
    for (const auto& category : m_categories.list()) {
        categoriesJson.append(QJsonObject{
            {QStringLiteral("name"), category.name},
        });
    }

    QJsonArray entriesJson;
    for (const auto& entry : m_entries.list()) {
        entriesJson.append(QJsonObject{
            {QStringLiteral("title"), entry.title},
            {QStringLiteral("category"), entry.categoryName},
            {QStringLiteral("username"), entry.username},
            {QStringLiteral("password"), entry.password},
            {QStringLiteral("url"), entry.url},
            {QStringLiteral("notes"), entry.notes},
        });
    }

    const QJsonObject payload{
        {QStringLiteral("format"), QStringLiteral("passkeeper-plaintext-export-v1")},
        {QStringLiteral("categories"), categoriesJson},
        {QStringLiteral("entries"), entriesJson},
    };

    const QByteArray salt = crypto::CryptoProvider::randomBytes(crypto::CryptoProvider::kSaltSize);
    const auto keys = crypto::CryptoProvider::deriveKeys(exportPassword, salt, kExportIterations);
    const QByteArray plaintext = QJsonDocument(payload).toJson(QJsonDocument::Compact);
    const auto encrypted = crypto::CryptoProvider::encrypt(
        plaintext,
        keys.encryptionKey,
        QByteArrayLiteral("passkeeper-export-v1"));

    const QJsonObject envelope{
        {QStringLiteral("magic"), QStringLiteral("PassKeeperExport")},
        {QStringLiteral("version"), 1},
        {QStringLiteral("kdf"), QStringLiteral("PBKDF2-HMAC-SHA256")},
        {QStringLiteral("iterations"), kExportIterations},
        {QStringLiteral("salt"), QString::fromLatin1(salt.toBase64())},
        {QStringLiteral("payload"), encodeBlob(encrypted)},
    };

    writeFile(filePath, QJsonDocument(envelope).toJson(QJsonDocument::Indented));
}

int ExportImportService::importEncrypted(const QString& filePath, const QString& exportPassword) const
{
    const QJsonDocument envelopeDoc = QJsonDocument::fromJson(readFile(filePath));
    const QJsonObject envelope = envelopeDoc.object();
    if (envelope.value(QStringLiteral("magic")).toString() != QStringLiteral("PassKeeperExport") ||
        envelope.value(QStringLiteral("version")).toInt() != 1) {
        throw std::runtime_error("unsupported export file");
    }

    const int iterations = envelope.value(QStringLiteral("iterations")).toInt();
    const QByteArray salt = QByteArray::fromBase64(envelope.value(QStringLiteral("salt")).toString().toLatin1());
    const auto keys = crypto::CryptoProvider::deriveKeys(exportPassword, salt, iterations);
    const auto encrypted = decodeBlob(envelope.value(QStringLiteral("payload")).toObject());
    const QByteArray plaintext = crypto::CryptoProvider::decrypt(
        encrypted,
        keys.encryptionKey,
        QByteArrayLiteral("passkeeper-export-v1"));

    const QJsonObject payload = QJsonDocument::fromJson(plaintext).object();
    if (payload.value(QStringLiteral("format")).toString() !=
        QStringLiteral("passkeeper-plaintext-export-v1")) {
        throw std::runtime_error("export payload integrity check failed");
    }

    m_database.exec(QStringLiteral("BEGIN"));

    try {
        for (const auto categoryValue : payload.value(QStringLiteral("categories")).toArray()) {
            m_categories.findOrCreate(categoryValue.toObject().value(QStringLiteral("name")).toString());
        }

        int imported = 0;
        for (const auto entryValue : payload.value(QStringLiteral("entries")).toArray()) {
            const QJsonObject object = entryValue.toObject();
            models::PasswordEntry entry;
            entry.title = object.value(QStringLiteral("title")).toString();
            entry.categoryId =
                m_categories.findOrCreate(object.value(QStringLiteral("category")).toString());
            entry.username = object.value(QStringLiteral("username")).toString();
            entry.password = object.value(QStringLiteral("password")).toString();
            entry.url = object.value(QStringLiteral("url")).toString();
            entry.notes = object.value(QStringLiteral("notes")).toString();
            m_entries.create(entry);
            ++imported;
        }

        m_database.exec(QStringLiteral("COMMIT"));
        return imported;
    } catch (...) {
        m_database.exec(QStringLiteral("ROLLBACK"));
        throw;
    }
}

} // namespace services
