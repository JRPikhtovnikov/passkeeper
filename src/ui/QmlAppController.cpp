#include "ui/QmlAppController.h"

#include "crypto/PasswordGenerator.h"
#include "services/ExportImportService.h"

#include <QUrl>

#include <functional>
#include <stdexcept>

QmlAppController::QmlAppController(database::DatabaseManager& database,
                                   services::AuthService& auth,
                                   services::CategoryService& categories,
                                   AppContext& context,
                                   QObject* parent)
    : QObject(parent)
    , m_database(database)
    , m_auth(auth)
    , m_categoriesService(categories)
    , m_context(context)
{
}

bool QmlAppController::firstRun() const
{
    return !m_auth.isMasterPasswordConfigured();
}

bool QmlAppController::unlocked() const
{
    return m_unlocked;
}

QString QmlAppController::errorMessage() const
{
    return m_errorMessage;
}

QVariantList QmlAppController::entries() const
{
    return m_entries;
}

QVariantList QmlAppController::categories() const
{
    return m_categories;
}

int QmlAppController::clipboardSeconds() const
{
    return m_context.clipboardClearSeconds;
}

QString QmlAppController::searchText() const
{
    return m_searchText;
}

qint64 QmlAppController::categoryFilter() const
{
    return m_categoryFilter;
}

bool QmlAppController::createMasterPassword(const QString& password, const QString& confirmation)
{
    return runSafely([&]() {
        if (password != confirmation) {
            throw std::runtime_error("passwords do not match");
        }
        m_auth.createMasterPassword(password);
        emit authStateChanged();
        unlock(password);
    });
}

bool QmlAppController::unlock(const QString& password)
{
    return runSafely([&]() {
        m_context.encryptionKey = m_auth.unlock(password);
        m_context.clipboardClearSeconds = m_auth.clipboardClearSeconds();
        m_entriesService = std::make_unique<services::EntryService>(m_database, m_context.encryptionKey);
        m_unlocked = true;
        refreshCategories();
        refreshEntries();
        emit authStateChanged();
        emit settingsChanged();
    });
}

void QmlAppController::reload()
{
    if (!m_unlocked) {
        return;
    }
    runSafely([&]() {
        refreshCategories();
        refreshEntries();
    });
}

bool QmlAppController::saveEntry(qint64 id,
                                 qint64 categoryId,
                                 const QString& title,
                                 const QString& username,
                                 const QString& password,
                                 const QString& url,
                                 const QString& notes)
{
    return runSafely([&]() {
        models::PasswordEntry entry;
        entry.id = id;
        entry.categoryId = categoryId;
        entry.title = title;
        entry.username = username;
        entry.password = password;
        entry.url = url;
        entry.notes = notes;
        if (id > 0) {
            m_entriesService->update(entry);
        } else {
            m_entriesService->create(entry);
        }
        refreshEntries();
    });
}

bool QmlAppController::deleteEntry(qint64 id)
{
    return runSafely([&]() {
        m_entriesService->remove(id);
        refreshEntries();
    });
}

bool QmlAppController::saveCategory(qint64 id, const QString& name)
{
    return runSafely([&]() {
        if (id > 0) {
            m_categoriesService.rename(id, name);
        } else {
            m_categoriesService.create(name);
        }
        refreshCategories();
        refreshEntries();
    });
}

bool QmlAppController::deleteCategory(qint64 id)
{
    return runSafely([&]() {
        m_categoriesService.remove(id);
        if (m_categoryFilter == id) {
            m_categoryFilter = -1;
            emit filtersChanged();
        }
        refreshCategories();
        refreshEntries();
    });
}

QString QmlAppController::generatePassword(int length,
                                           bool lowercase,
                                           bool uppercase,
                                           bool digits,
                                           bool symbols)
{
    try {
        crypto::PasswordOptions options;
        options.length = length;
        options.lowercase = lowercase;
        options.uppercase = uppercase;
        options.digits = digits;
        options.symbols = symbols;
        return crypto::PasswordGenerator::generate(options);
    } catch (const std::exception& error) {
        setError(QString::fromUtf8(error.what()));
        return {};
    }
}

void QmlAppController::copyPassword(qint64 id)
{
    for (const auto& entry : m_entryCache) {
        if (entry.id == id) {
            m_clipboard.copySecret(entry.password, m_context.clipboardClearSeconds);
            emit toast(tr("Password copied"));
            return;
        }
    }
}

bool QmlAppController::exportVault(const QString& fileUrl, const QString& password)
{
    return runSafely([&]() {
        services::ExportImportService service(m_database, *m_entriesService, m_categoriesService);
        service.exportEncrypted(localPathFromUrl(fileUrl), password);
        emit toast(tr("Vault exported"));
    });
}

bool QmlAppController::importVault(const QString& fileUrl, const QString& password)
{
    return runSafely([&]() {
        services::ExportImportService service(m_database, *m_entriesService, m_categoriesService);
        const int count = service.importEncrypted(localPathFromUrl(fileUrl), password);
        refreshCategories();
        refreshEntries();
        emit toast(tr("Imported %1 entries").arg(count));
    });
}

QVariantMap QmlAppController::entryById(qint64 id) const
{
    for (const auto& entry : m_entryCache) {
        if (entry.id == id) {
            return entryToVariant(entry);
        }
    }
    return {};
}

void QmlAppController::clearError()
{
    setError({});
}

void QmlAppController::setClipboardSeconds(int seconds)
{
    runSafely([&]() {
        m_auth.setClipboardClearSeconds(seconds);
        m_context.clipboardClearSeconds = seconds;
        emit settingsChanged();
    });
}

void QmlAppController::setSearchText(const QString& searchText)
{
    if (m_searchText == searchText) {
        return;
    }
    m_searchText = searchText;
    emit filtersChanged();
    refreshEntries();
}

void QmlAppController::setCategoryFilter(qint64 categoryFilter)
{
    if (m_categoryFilter == categoryFilter) {
        return;
    }
    m_categoryFilter = categoryFilter;
    emit filtersChanged();
    refreshEntries();
}

QVariantMap QmlAppController::entryToVariant(const models::PasswordEntry& entry) const
{
    return {
        {QStringLiteral("id"), entry.id},
        {QStringLiteral("categoryId"), entry.categoryId},
        {QStringLiteral("categoryName"), entry.categoryName},
        {QStringLiteral("title"), entry.title},
        {QStringLiteral("username"), entry.username},
        {QStringLiteral("password"), entry.password},
        {QStringLiteral("url"), entry.url},
        {QStringLiteral("notes"), entry.notes},
    };
}

QVariantMap QmlAppController::categoryToVariant(const models::Category& category) const
{
    return {
        {QStringLiteral("id"), category.id},
        {QStringLiteral("name"), category.name},
    };
}

QString QmlAppController::localPathFromUrl(const QString& fileUrl) const
{
    const QUrl url(fileUrl);
    return url.isLocalFile() ? url.toLocalFile() : fileUrl;
}

bool QmlAppController::runSafely(const std::function<void()>& action)
{
    try {
        clearError();
        action();
        return true;
    } catch (const std::exception& error) {
        setError(QString::fromUtf8(error.what()));
        return false;
    }
}

void QmlAppController::setError(const QString& message)
{
    if (m_errorMessage == message) {
        return;
    }
    m_errorMessage = message;
    emit errorMessageChanged();
}

void QmlAppController::refreshCategories()
{
    m_categories.clear();
    m_categories.push_back(QVariantMap{
        {QStringLiteral("id"), -1},
        {QStringLiteral("name"), tr("All")},
    });
    for (const auto& category : m_categoriesService.list()) {
        m_categories.push_back(categoryToVariant(category));
    }
    emit categoriesChanged();
}

void QmlAppController::refreshEntries()
{
    if (!m_entriesService) {
        return;
    }

    m_entryCache.clear();
    m_entries.clear();
    for (const auto& entry : m_entriesService->list(m_searchText)) {
        if (m_categoryFilter <= 0 || entry.categoryId == m_categoryFilter) {
            m_entryCache.push_back(entry);
            m_entries.push_back(entryToVariant(entry));
        }
    }
    emit entriesChanged();
}

