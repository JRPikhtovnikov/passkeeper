#pragma once

#include "models/CryptoTypes.h"

#include <QByteArray>
#include <QString>

namespace crypto {

class CryptoProvider {
public:
    static constexpr int kSaltSize = 16;
    static constexpr int kAesKeySize = 32;
    static constexpr int kNonceSize = 12;
    static constexpr int kTagSize = 16;
    static constexpr int kDefaultPbkdf2Iterations = 600000;

    static QByteArray randomBytes(int size);
    static models::DerivedKeys deriveKeys(const QString& password,
                                          const QByteArray& salt,
                                          int iterations);
    static QByteArray verifierHash(const QByteArray& verifierKey);
    static bool constantTimeEquals(const QByteArray& left, const QByteArray& right);

    static models::EncryptedBlob encrypt(const QByteArray& plaintext,
                                         const QByteArray& key,
                                         const QByteArray& aad = {});
    static QByteArray decrypt(const models::EncryptedBlob& blob,
                              const QByteArray& key,
                              const QByteArray& aad = {});
};

} // namespace crypto

