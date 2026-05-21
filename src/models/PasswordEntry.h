#pragma once

#include <QDateTime>
#include <QString>

namespace models {

struct PasswordEntry {
    qint64 id = -1;
    qint64 categoryId = -1;
    QString categoryName;
    QString title;
    QString username;
    QString password;
    QString url;
    QString notes;
    QDateTime createdAt;
    QDateTime updatedAt;
};

} // namespace models

