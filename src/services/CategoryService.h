#pragma once

#include "database/DatabaseManager.h"
#include "models/Category.h"

#include <QList>
#include <QString>

namespace services {

class CategoryService {
public:
    explicit CategoryService(database::DatabaseManager& database);

    QList<models::Category> list() const;
    qint64 create(const QString& name) const;
    void rename(qint64 id, const QString& name) const;
    void remove(qint64 id) const;
    qint64 findOrCreate(const QString& name) const;

private:
    database::DatabaseManager& m_database;
};

} // namespace services

