#include "utils/Env.h"

#include <QByteArray>
#include <cstdlib>

namespace utils {

QString envOrDefault(const char* name, const QString& fallback)
{
    const char* value = std::getenv(name);
    if (value == nullptr || QByteArray(value).isEmpty()) {
        return fallback;
    }
    return QString::fromUtf8(value);
}

int envIntOrDefault(const char* name, int fallback)
{
    bool ok = false;
    const int value = envOrDefault(name, QString::number(fallback)).toInt(&ok);
    return ok ? value : fallback;
}

} // namespace utils

