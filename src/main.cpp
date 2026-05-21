#include "core/AppContext.h"
#include "database/DatabaseManager.h"
#include "services/AuthService.h"
#include "services/CategoryService.h"
#include "ui/QmlAppController.h"
#include "utils/DotEnv.h"
#include "utils/Env.h"

#include <QDir>
#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQuickStyle>

int main(int argc, char* argv[])
{
    QGuiApplication app(argc, argv);
    QGuiApplication::setApplicationName(QStringLiteral("PassKeeper"));
    QGuiApplication::setOrganizationName(QStringLiteral("PassKeeper"));
    QQuickStyle::setStyle(QStringLiteral("Basic"));

    utils::loadDotEnv(QCoreApplication::applicationDirPath() + QStringLiteral("/.env"));
    utils::loadDotEnv(QDir::currentPath() + QStringLiteral("/.env"));

    try {
        database::DatabaseConfig config;
        config.host = utils::envOrDefault("PASSKEEPER_DB_HOST", config.host);
        config.port = utils::envIntOrDefault("PASSKEEPER_DB_PORT", config.port);
        config.databaseName = utils::envOrDefault("PASSKEEPER_DB_NAME", config.databaseName);
        config.user = utils::envOrDefault("PASSKEEPER_DB_USER", config.user);
        config.password = utils::envOrDefault("PASSKEEPER_DB_PASSWORD", config.password);

        database::DatabaseManager database;
        database.connect(config);
        database.ensureSchema();

        AppContext context;
        services::AuthService auth(database);
        services::CategoryService categories(database);
        QmlAppController controller(database, auth, categories, context);

        QQmlApplicationEngine engine;
        engine.rootContext()->setContextProperty(QStringLiteral("appController"), &controller);

        const QUrl url(QStringLiteral("qrc:/qml/Main.qml"));
        QObject::connect(&engine, &QQmlApplicationEngine::objectCreated, &app, [url](QObject* object, const QUrl& objectUrl) {
            if (object == nullptr && url == objectUrl) {
                QCoreApplication::exit(1);
            }
        });
        engine.load(url);
        return QGuiApplication::exec();
    } catch (const std::exception& error) {
        qCritical("PassKeeper startup failed: %s", error.what());
        return 1;
    }
}

