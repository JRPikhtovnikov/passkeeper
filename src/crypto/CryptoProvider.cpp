#include "crypto/CryptoProvider.h"

#include <QCryptographicHash>

#include <openssl/crypto.h>
#include <openssl/err.h>
#include <openssl/evp.h>
#include <openssl/rand.h>

#include <memory>
#include <stdexcept>

namespace {

using EvpCipherCtx = std::unique_ptr<EVP_CIPHER_CTX, decltype(&EVP_CIPHER_CTX_free)>;

std::runtime_error opensslError(const char* action)
{
    const unsigned long error = ERR_get_error();
    if (error == 0) {
        return std::runtime_error(action);
    }

    char buffer[256] = {};
    ERR_error_string_n(error, buffer, sizeof(buffer));
    return std::runtime_error(QString("%1: %2").arg(action, buffer).toStdString());
}

} // namespace

namespace crypto {

QByteArray CryptoProvider::randomBytes(int size)
{
    if (size <= 0) {
        throw std::invalid_argument("random byte count must be positive");
    }

    QByteArray bytes(size, Qt::Uninitialized);
    if (RAND_bytes(reinterpret_cast<unsigned char*>(bytes.data()), size) != 1) {
        throw opensslError("RAND_bytes failed");
    }
    return bytes;
}

models::DerivedKeys CryptoProvider::deriveKeys(const QString& password,
                                               const QByteArray& salt,
                                               int iterations)
{
    if (password.size() < 8) {
        throw std::invalid_argument("master password must contain at least 8 characters");
    }
    if (salt.size() < kSaltSize) {
        throw std::invalid_argument("salt is too short");
    }
    if (iterations < 100000) {
        throw std::invalid_argument("PBKDF2 iteration count is too low");
    }

    QByteArray passwordBytes = password.toUtf8();
    QByteArray output(kAesKeySize * 2, Qt::Uninitialized);

    const int ok = PKCS5_PBKDF2_HMAC(passwordBytes.constData(),
                                     passwordBytes.size(),
                                     reinterpret_cast<const unsigned char*>(salt.constData()),
                                     salt.size(),
                                     iterations,
                                     EVP_sha256(),
                                     output.size(),
                                     reinterpret_cast<unsigned char*>(output.data()));
    OPENSSL_cleanse(passwordBytes.data(), passwordBytes.size());

    if (ok != 1) {
        throw opensslError("PBKDF2 failed");
    }

    return {
        output.left(kAesKeySize),
        output.mid(kAesKeySize, kAesKeySize),
    };
}

QByteArray CryptoProvider::verifierHash(const QByteArray& verifierKey)
{
    QByteArray input = QByteArrayLiteral("passkeeper-master-verifier-v1:") + verifierKey;
    QByteArray hash = QCryptographicHash::hash(input, QCryptographicHash::Sha256);
    OPENSSL_cleanse(input.data(), input.size());
    return hash;
}

bool CryptoProvider::constantTimeEquals(const QByteArray& left, const QByteArray& right)
{
    if (left.size() != right.size()) {
        return false;
    }
    return CRYPTO_memcmp(left.constData(), right.constData(), left.size()) == 0;
}

models::EncryptedBlob CryptoProvider::encrypt(const QByteArray& plaintext,
                                              const QByteArray& key,
                                              const QByteArray& aad)
{
    if (key.size() != kAesKeySize) {
        throw std::invalid_argument("AES-256-GCM key must be 32 bytes");
    }

    models::EncryptedBlob blob;
    blob.nonce = randomBytes(kNonceSize);
    blob.ciphertext.resize(plaintext.size());
    blob.tag.resize(kTagSize);

    EvpCipherCtx ctx(EVP_CIPHER_CTX_new(), EVP_CIPHER_CTX_free);
    if (!ctx) {
        throw std::runtime_error("EVP_CIPHER_CTX_new failed");
    }

    int len = 0;
    if (EVP_EncryptInit_ex(ctx.get(), EVP_aes_256_gcm(), nullptr, nullptr, nullptr) != 1 ||
        EVP_CIPHER_CTX_ctrl(ctx.get(), EVP_CTRL_GCM_SET_IVLEN, blob.nonce.size(), nullptr) != 1 ||
        EVP_EncryptInit_ex(ctx.get(),
                           nullptr,
                           nullptr,
                           reinterpret_cast<const unsigned char*>(key.constData()),
                           reinterpret_cast<const unsigned char*>(blob.nonce.constData())) != 1) {
        throw opensslError("AES-GCM encryption init failed");
    }

    if (!aad.isEmpty() &&
        EVP_EncryptUpdate(ctx.get(),
                          nullptr,
                          &len,
                          reinterpret_cast<const unsigned char*>(aad.constData()),
                          aad.size()) != 1) {
        throw opensslError("AES-GCM AAD update failed");
    }

    if (!plaintext.isEmpty() &&
        EVP_EncryptUpdate(ctx.get(),
                          reinterpret_cast<unsigned char*>(blob.ciphertext.data()),
                          &len,
                          reinterpret_cast<const unsigned char*>(plaintext.constData()),
                          plaintext.size()) != 1) {
        throw opensslError("AES-GCM encryption failed");
    }

    if (EVP_EncryptFinal_ex(ctx.get(),
                            reinterpret_cast<unsigned char*>(blob.ciphertext.data()) + len,
                            &len) != 1 ||
        EVP_CIPHER_CTX_ctrl(ctx.get(),
                            EVP_CTRL_GCM_GET_TAG,
                            blob.tag.size(),
                            reinterpret_cast<unsigned char*>(blob.tag.data())) != 1) {
        throw opensslError("AES-GCM finalize failed");
    }

    return blob;
}

QByteArray CryptoProvider::decrypt(const models::EncryptedBlob& blob,
                                   const QByteArray& key,
                                   const QByteArray& aad)
{
    if (key.size() != kAesKeySize) {
        throw std::invalid_argument("AES-256-GCM key must be 32 bytes");
    }
    if (blob.nonce.size() != kNonceSize || blob.tag.size() != kTagSize) {
        throw std::invalid_argument("invalid AES-GCM nonce or tag size");
    }

    QByteArray plaintext(blob.ciphertext.size(), Qt::Uninitialized);
    EvpCipherCtx ctx(EVP_CIPHER_CTX_new(), EVP_CIPHER_CTX_free);
    if (!ctx) {
        throw std::runtime_error("EVP_CIPHER_CTX_new failed");
    }

    int len = 0;
    if (EVP_DecryptInit_ex(ctx.get(), EVP_aes_256_gcm(), nullptr, nullptr, nullptr) != 1 ||
        EVP_CIPHER_CTX_ctrl(ctx.get(), EVP_CTRL_GCM_SET_IVLEN, blob.nonce.size(), nullptr) != 1 ||
        EVP_DecryptInit_ex(ctx.get(),
                           nullptr,
                           nullptr,
                           reinterpret_cast<const unsigned char*>(key.constData()),
                           reinterpret_cast<const unsigned char*>(blob.nonce.constData())) != 1) {
        throw opensslError("AES-GCM decryption init failed");
    }

    if (!aad.isEmpty() &&
        EVP_DecryptUpdate(ctx.get(),
                          nullptr,
                          &len,
                          reinterpret_cast<const unsigned char*>(aad.constData()),
                          aad.size()) != 1) {
        throw opensslError("AES-GCM AAD update failed");
    }

    if (!blob.ciphertext.isEmpty() &&
        EVP_DecryptUpdate(ctx.get(),
                          reinterpret_cast<unsigned char*>(plaintext.data()),
                          &len,
                          reinterpret_cast<const unsigned char*>(blob.ciphertext.constData()),
                          blob.ciphertext.size()) != 1) {
        throw opensslError("AES-GCM decryption failed");
    }

    if (EVP_CIPHER_CTX_ctrl(ctx.get(),
                            EVP_CTRL_GCM_SET_TAG,
                            blob.tag.size(),
                            const_cast<char*>(blob.tag.constData())) != 1 ||
        EVP_DecryptFinal_ex(ctx.get(),
                            reinterpret_cast<unsigned char*>(plaintext.data()) + len,
                            &len) != 1) {
        throw std::runtime_error("AES-GCM authentication failed");
    }

    return plaintext;
}

} // namespace crypto

