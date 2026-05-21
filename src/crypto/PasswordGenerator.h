#pragma once

#include <QString>

namespace crypto {

struct PasswordOptions {
    int length = 20;
    bool lowercase = true;
    bool uppercase = true;
    bool digits = true;
    bool symbols = true;
};

class PasswordGenerator {
public:
    static QString generate(const PasswordOptions& options);
};

} // namespace crypto

