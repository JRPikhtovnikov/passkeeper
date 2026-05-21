#include "crypto/PasswordGenerator.h"

#include "crypto/CryptoProvider.h"

#include <QStringList>
#include <algorithm>
#include <cstring>
#include <limits>
#include <stdexcept>
#include <vector>

namespace {

int secureIndex(int upperExclusive)
{
    if (upperExclusive <= 0) {
        throw std::invalid_argument("upper bound must be positive");
    }

    const quint32 bound = static_cast<quint32>(upperExclusive);
    const quint32 limit = std::numeric_limits<quint32>::max() -
                          (std::numeric_limits<quint32>::max() % bound);

    quint32 value = 0;
    do {
        const QByteArray bytes = crypto::CryptoProvider::randomBytes(sizeof(quint32));
        std::memcpy(&value, bytes.constData(), sizeof(value));
    } while (value >= limit);

    return static_cast<int>(value % bound);
}

QChar pickOne(const QString& alphabet)
{
    return alphabet.at(secureIndex(alphabet.size()));
}

} // namespace

namespace crypto {

QString PasswordGenerator::generate(const PasswordOptions& options)
{
    if (options.length < 8 || options.length > 256) {
        throw std::invalid_argument("password length must be between 8 and 256");
    }

    const QString lower = QStringLiteral("abcdefghijklmnopqrstuvwxyz");
    const QString upper = QStringLiteral("ABCDEFGHIJKLMNOPQRSTUVWXYZ");
    const QString digits = QStringLiteral("0123456789");
    const QString symbols = QStringLiteral("!@#$%^&*()-_=+[]{};:,.?/|~");

    QStringList groups;
    QString alphabet;
    if (options.lowercase) {
        groups << lower;
        alphabet += lower;
    }
    if (options.uppercase) {
        groups << upper;
        alphabet += upper;
    }
    if (options.digits) {
        groups << digits;
        alphabet += digits;
    }
    if (options.symbols) {
        groups << symbols;
        alphabet += symbols;
    }

    if (groups.isEmpty()) {
        throw std::invalid_argument("at least one character group must be selected");
    }
    if (options.length < groups.size()) {
        throw std::invalid_argument("length is too short for selected character groups");
    }

    QString password;
    password.reserve(options.length);
    for (const QString& group : groups) {
        password.append(pickOne(group));
    }
    while (password.size() < options.length) {
        password.append(pickOne(alphabet));
    }

    std::vector<QChar> chars(password.begin(), password.end());
    for (int i = static_cast<int>(chars.size()) - 1; i > 0; --i) {
        std::swap(chars[static_cast<size_t>(i)], chars[static_cast<size_t>(secureIndex(i + 1))]);
    }

    QString shuffled;
    shuffled.reserve(options.length);
    for (const QChar ch : chars) {
        shuffled.append(ch);
    }
    return shuffled;
}

} // namespace crypto
