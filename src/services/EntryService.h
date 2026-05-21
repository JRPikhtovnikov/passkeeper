#pragma once

#include "database/DatabaseManager.h"
#include "models/CryptoTypes.h"
#include "models/PasswordEntry.h"

#include <QByteArray>
#include <QList>
#include <QString>

namespace services {

class EntryService {
public:
    EntryService(database::DatabaseManager& database, QByteArray encryptionKey);

    QList<models::PasswordEntry> list(const QString& filter = {}) const;
    qint64 create(const models::PasswordEntry& entry) const;
    void update(const models::PasswordEntry& entry) const;
    void remove(qint64 id) const;

private:
    models::EncryptedBlob encryptField(qint64 entryIdHint,
                                       const QString& field,
                                       const QString& value) const;
    QString decryptField(qint64 entryId, const QString& field, const models::EncryptedBlob& blob) const;
    static QByteArray aad(qint64 entryId, const QString& field);
    static bool matchesFilter(const models::PasswordEntry& entry, const QString& normalizedFilter);

    database::DatabaseManager& m_database;
    QByteArray m_encryptionKey;
};

} // namespace services

