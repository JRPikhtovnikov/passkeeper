#include "services/AuthService.h"

#include "crypto/CryptoProvider.h"

#include <stdexcept>

namespace services {

AuthService::AuthService(database::DatabaseManager& database)
    : m_database(database)
{
}

bool AuthService::isMasterPasswordConfigured() const
{
    return !setting(QStringLiteral("master_salt")).isEmpty() &&
           !setting(QStringLiteral("master_verifier")).isEmpty();
}

void AuthService::createMasterPassword(const QString& password)
{
    if (isMasterPasswordConfigured()) {
        throw std::runtime_error("master password already configured");
    }

    const QByteArray salt = crypto::CryptoProvider::randomBytes(crypto::CryptoProvider::kSaltSize);
    const int iterations = crypto::CryptoProvider::kDefaultPbkdf2Iterations;
    const auto keys = crypto::CryptoProvider::deriveKeys(password, salt, iterations);
    const QByteArray verifier = crypto::CryptoProvider::verifierHash(keys.verifierKey);

    upsertSetting(QStringLiteral("kdf"), QStringLiteral("PBKDF2-HMAC-SHA256"));
    upsertSetting(QStringLiteral("kdf_iterations"), QString::number(iterations));
    upsertSetting(QStringLiteral("master_salt"), QString::fromLatin1(salt.toBase64()));
    upsertSetting(QStringLiteral("master_verifier"), QString::fromLatin1(verifier.toBase64()));
    upsertSetting(QStringLiteral("clipboard_clear_seconds"), QStringLiteral("30"));
}

QByteArray AuthService::unlock(const QString& password) const
{
    const QByteArray salt = QByteArray::fromBase64(setting(QStringLiteral("master_salt")).toLatin1());
    const QByteArray expectedVerifier =
        QByteArray::fromBase64(setting(QStringLiteral("master_verifier")).toLatin1());

    bool ok = false;
    const int iterations = setting(QStringLiteral("kdf_iterations")).toInt(&ok);
    if (!ok || salt.isEmpty() || expectedVerifier.isEmpty()) {
        throw std::runtime_error("master password metadata is corrupted");
    }

    const auto keys = crypto::CryptoProvider::deriveKeys(password, salt, iterations);
    const QByteArray actualVerifier = crypto::CryptoProvider::verifierHash(keys.verifierKey);
    if (!crypto::CryptoProvider::constantTimeEquals(actualVerifier, expectedVerifier)) {
        throw std::runtime_error("invalid master password");
    }
    return keys.encryptionKey;
}

int AuthService::clipboardClearSeconds() const
{
    bool ok = false;
    const int value = setting(QStringLiteral("clipboard_clear_seconds")).toInt(&ok);
    return ok && value >= 5 && value <= 300 ? value : 30;
}

void AuthService::setClipboardClearSeconds(int seconds)
{
    if (seconds < 5 || seconds > 300) {
        throw std::invalid_argument("clipboard timeout must be between 5 and 300 seconds");
    }
    upsertSetting(QStringLiteral("clipboard_clear_seconds"), QString::number(seconds));
}

QString AuthService::setting(const QString& key) const
{
    const auto result = m_database.execParams(
        QStringLiteral("SELECT value FROM app_settings WHERE key = $1"),
        {database::DatabaseManager::Param::text(key)});
    if (result.empty()) {
        return {};
    }
    return result.value(0, 0);
}

void AuthService::upsertSetting(const QString& key, const QString& value) const
{
    m_database.execParams(
        QStringLiteral(
            "INSERT INTO app_settings(key, value) VALUES($1, $2) "
            "ON CONFLICT(key) DO UPDATE SET value = excluded.value"),
        {
            database::DatabaseManager::Param::text(key),
            database::DatabaseManager::Param::text(value),
        });
}

} // namespace services
