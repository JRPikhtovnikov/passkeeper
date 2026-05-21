#include "services/CategoryService.h"

#include <stdexcept>

namespace services {

CategoryService::CategoryService(database::DatabaseManager& database)
    : m_database(database)
{
}

QList<models::Category> CategoryService::list() const
{
    const auto result = m_database.exec(QStringLiteral(
        "SELECT id, name, created_at, updated_at FROM categories ORDER BY lower(name)"));

    QList<models::Category> categories;
    for (int row = 0; row < result.rows(); ++row) {
        categories.push_back({
            result.int64(row, 0),
            result.value(row, 1),
            result.timestamp(row, 2),
            result.timestamp(row, 3),
        });
    }
    return categories;
}

qint64 CategoryService::create(const QString& name) const
{
    const QString trimmed = name.trimmed();
    if (trimmed.isEmpty()) {
        throw std::invalid_argument("category name is required");
    }

    const auto result = m_database.execParams(
        QStringLiteral("INSERT INTO categories(name) VALUES($1) RETURNING id"),
        {database::DatabaseManager::Param::text(trimmed)});
    if (result.empty()) {
        throw std::runtime_error("category creation did not return id");
    }
    return result.int64(0, 0);
}

void CategoryService::rename(qint64 id, const QString& name) const
{
    const QString trimmed = name.trimmed();
    if (id <= 0 || trimmed.isEmpty()) {
        throw std::invalid_argument("valid category id and name are required");
    }

    m_database.execParams(
        QStringLiteral("UPDATE categories SET name = $1, updated_at = now() WHERE id = $2"),
        {
            database::DatabaseManager::Param::text(trimmed),
            database::DatabaseManager::Param::int64(id),
        });
}

void CategoryService::remove(qint64 id) const
{
    if (id <= 0) {
        throw std::invalid_argument("valid category id is required");
    }

    m_database.execParams(QStringLiteral("DELETE FROM categories WHERE id = $1"),
                          {database::DatabaseManager::Param::int64(id)});
}

qint64 CategoryService::findOrCreate(const QString& name) const
{
    const QString trimmed = name.trimmed();
    if (trimmed.isEmpty()) {
        return -1;
    }

    const auto result = m_database.execParams(
        QStringLiteral("SELECT id FROM categories WHERE lower(name) = lower($1)"),
        {database::DatabaseManager::Param::text(trimmed)});
    if (!result.empty()) {
        return result.int64(0, 0);
    }
    return create(trimmed);
}

} // namespace services
