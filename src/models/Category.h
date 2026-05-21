#pragma once

#include <QDateTime>
#include <QString>

namespace models {

struct Category {
    qint64 id = -1;
    QString name;
    QDateTime createdAt;
    QDateTime updatedAt;
};

} // namespace models

