#pragma once

#include <QByteArray>

namespace models {

struct EncryptedBlob {
    QByteArray nonce;
    QByteArray ciphertext;
    QByteArray tag;
};

struct DerivedKeys {
    QByteArray encryptionKey;
    QByteArray verifierKey;
};

} // namespace models

