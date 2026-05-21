#pragma once

#include "database/DatabaseManager.h"

#include <QByteArray>
#include <QString>

namespace services {

class AuthService {
public:
    explicit AuthService(database::DatabaseManager& database);

    bool isMasterPasswordConfigured() const;
    void createMasterPassword(const QString& password);
    QByteArray unlock(const QString& password) const;
    int clipboardClearSeconds() const;
    void setClipboardClearSeconds(int seconds);

private:
    QString setting(const QString& key) const;
    void upsertSetting(const QString& key, const QString& value) const;

    database::DatabaseManager& m_database;
};

} // namespace services

