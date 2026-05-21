#pragma once

#include <QString>

namespace utils {

QString envOrDefault(const char* name, const QString& fallback);
int envIntOrDefault(const char* name, int fallback);

} // namespace utils

