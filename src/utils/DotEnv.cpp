#include "utils/DotEnv.h"

#include <QFile>
#include <QRegularExpression>
#include <QStringList>

#include <cstdlib>

namespace {

QString unquote(QString value)
{
    value = value.trimmed();
    if (value.size() >= 2 &&
        ((value.startsWith('"') && value.endsWith('"')) ||
         (value.startsWith('\'') && value.endsWith('\'')))) {
        value = value.mid(1, value.size() - 2);
    }
    return value;
}

} // namespace

namespace utils {

void loadDotEnv(const QString& filePath)
{
    QFile file(filePath);
    if (!file.exists() || !file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return;
    }

    while (!file.atEnd()) {
        QString line = QString::fromUtf8(file.readLine()).trimmed();
        if (line.isEmpty() || line.startsWith('#')) {
            continue;
        }
        if (line.startsWith(QStringLiteral("export "))) {
            line = line.mid(7).trimmed();
        }

        const qsizetype equals = line.indexOf('=');
        if (equals <= 0) {
            continue;
        }

        const QString key = line.left(equals).trimmed();
        const QString value = unquote(line.mid(equals + 1));
        if (!QRegularExpression(QStringLiteral("^[A-Za-z_][A-Za-z0-9_]*$")).match(key).hasMatch()) {
            continue;
        }

        qputenv(key.toUtf8().constData(), value.toUtf8());
    }
}

} // namespace utils

