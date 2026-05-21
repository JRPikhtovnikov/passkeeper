#pragma once

#include "core/AppContext.h"
#include "database/DatabaseManager.h"
#include "models/PasswordEntry.h"
#include "services/AuthService.h"
#include "services/CategoryService.h"
#include "services/ClipboardService.h"
#include "services/EntryService.h"

#include <QObject>
#include <QVariantList>

#include <functional>
#include <memory>

class QmlAppController final : public QObject {
    Q_OBJECT
    Q_PROPERTY(bool firstRun READ firstRun NOTIFY authStateChanged)
    Q_PROPERTY(bool unlocked READ unlocked NOTIFY authStateChanged)
    Q_PROPERTY(QString errorMessage READ errorMessage NOTIFY errorMessageChanged)
    Q_PROPERTY(QVariantList entries READ entries NOTIFY entriesChanged)
    Q_PROPERTY(QVariantList categories READ categories NOTIFY categoriesChanged)
    Q_PROPERTY(int clipboardSeconds READ clipboardSeconds WRITE setClipboardSeconds NOTIFY settingsChanged)
    Q_PROPERTY(QString searchText READ searchText WRITE setSearchText NOTIFY filtersChanged)
    Q_PROPERTY(qint64 categoryFilter READ categoryFilter WRITE setCategoryFilter NOTIFY filtersChanged)

public:
    QmlAppController(database::DatabaseManager& database,
                     services::AuthService& auth,
                     services::CategoryService& categories,
                     AppContext& context,
                     QObject* parent = nullptr);

    bool firstRun() const;
    bool unlocked() const;
    QString errorMessage() const;
    QVariantList entries() const;
    QVariantList categories() const;
    int clipboardSeconds() const;
    QString searchText() const;
    qint64 categoryFilter() const;

    Q_INVOKABLE bool createMasterPassword(const QString& password, const QString& confirmation);
    Q_INVOKABLE bool unlock(const QString& password);
    Q_INVOKABLE void reload();
    Q_INVOKABLE bool saveEntry(qint64 id,
                               qint64 categoryId,
                               const QString& title,
                               const QString& username,
                               const QString& password,
                               const QString& url,
                               const QString& notes);
    Q_INVOKABLE bool deleteEntry(qint64 id);
    Q_INVOKABLE bool saveCategory(qint64 id, const QString& name);
    Q_INVOKABLE bool deleteCategory(qint64 id);
    Q_INVOKABLE QString generatePassword(int length,
                                         bool lowercase,
                                         bool uppercase,
                                         bool digits,
                                         bool symbols);
    Q_INVOKABLE void copyPassword(qint64 id);
    Q_INVOKABLE bool exportVault(const QString& fileUrl, const QString& password);
    Q_INVOKABLE bool importVault(const QString& fileUrl, const QString& password);
    Q_INVOKABLE QVariantMap entryById(qint64 id) const;
    Q_INVOKABLE void clearError();

public slots:
    void setClipboardSeconds(int seconds);
    void setSearchText(const QString& searchText);
    void setCategoryFilter(qint64 categoryFilter);

signals:
    void authStateChanged();
    void errorMessageChanged();
    void entriesChanged();
    void categoriesChanged();
    void filtersChanged();
    void settingsChanged();
    void toast(QString message);

private:
    QVariantMap entryToVariant(const models::PasswordEntry& entry) const;
    QVariantMap categoryToVariant(const models::Category& category) const;
    QString localPathFromUrl(const QString& fileUrl) const;
    bool runSafely(const std::function<void()>& action);
    void setError(const QString& message);
    void refreshCategories();
    void refreshEntries();

    database::DatabaseManager& m_database;
    services::AuthService& m_auth;
    services::CategoryService& m_categoriesService;
    AppContext& m_context;
    services::ClipboardService m_clipboard;
    std::unique_ptr<services::EntryService> m_entriesService;
    QVariantList m_entries;
    QVariantList m_categories;
    QList<models::PasswordEntry> m_entryCache;
    QString m_errorMessage;
    QString m_searchText;
    qint64 m_categoryFilter = -1;
    bool m_unlocked = false;
};
