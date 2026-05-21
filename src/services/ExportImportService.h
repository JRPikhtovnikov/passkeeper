#pragma once

#include "database/DatabaseManager.h"
#include "services/CategoryService.h"
#include "services/EntryService.h"

#include <QString>

namespace services {

class ExportImportService {
public:
    ExportImportService(database::DatabaseManager& database,
                        EntryService& entries,
                        CategoryService& categories);

    void exportEncrypted(const QString& filePath, const QString& exportPassword) const;
    int importEncrypted(const QString& filePath, const QString& exportPassword) const;

private:
    database::DatabaseManager& m_database;
    EntryService& m_entries;
    CategoryService& m_categories;
};

} // namespace services

