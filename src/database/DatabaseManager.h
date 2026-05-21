#pragma once

#include <QByteArray>
#include <QDateTime>
#include <QList>
#include <QString>

#include <libpq-fe.h>
#include <memory>

namespace database {

struct DatabaseConfig {
    QString host = QStringLiteral("localhost");
    int port = 5432;
    QString databaseName = QStringLiteral("passkeeper");
    QString user = QStringLiteral("postgres");
    QString password;
};

class DatabaseManager {
public:
    explicit DatabaseManager(QString connectionName = QStringLiteral("passkeeper"));
    ~DatabaseManager();

    DatabaseManager(const DatabaseManager&) = delete;
    DatabaseManager& operator=(const DatabaseManager&) = delete;

    void connect(const DatabaseConfig& config);
    void ensureSchema();
    PGconn* connection() const;

    class Result {
    public:
        explicit Result(PGresult* result = nullptr);
        ~Result();

        Result(const Result&) = delete;
        Result& operator=(const Result&) = delete;
        Result(Result&& other) noexcept;
        Result& operator=(Result&& other) noexcept;

        int rows() const;
        bool empty() const;
        QString value(int row, int column) const;
        QByteArray bytea(int row, int column) const;
        qint64 int64(int row, int column) const;
        QDateTime timestamp(int row, int column) const;
        PGresult* raw() const;

    private:
        PGresult* m_result = nullptr;
    };

    struct Param {
        QByteArray value;
        Oid type = 0;
        int format = 0;

        static Param text(const QString& value);
        static Param int64(qint64 value);
        static Param bytea(const QByteArray& value);
    };

    Result exec(const QString& sql) const;
    Result execParams(const QString& sql, const QList<Param>& params) const;

    static QString schemaSql();

private:
    static QString conninfoQuote(const QString& value);
    void checkResult(PGresult* result, const QString& context) const;

    PGconn* m_connection = nullptr;
};

} // namespace database
