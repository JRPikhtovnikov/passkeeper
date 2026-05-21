#include "database/DatabaseManager.h"

#include <QRegularExpression>

#include <stdexcept>
#include <vector>

namespace database {

DatabaseManager::DatabaseManager(QString connectionName)
{
    Q_UNUSED(connectionName);
}

DatabaseManager::~DatabaseManager()
{
    if (m_connection != nullptr) {
        PQfinish(m_connection);
    }
}

void DatabaseManager::connect(const DatabaseConfig& config)
{
    const QString conninfo = QStringLiteral("host=%1 port=%2 dbname=%3 user=%4 password=%5")
                                 .arg(conninfoQuote(config.host),
                                      QString::number(config.port),
                                      conninfoQuote(config.databaseName),
                                      conninfoQuote(config.user),
                                      conninfoQuote(config.password));

    m_connection = PQconnectdb(conninfo.toUtf8().constData());
    if (m_connection == nullptr || PQstatus(m_connection) != CONNECTION_OK) {
        const QString message = m_connection != nullptr
            ? QString::fromUtf8(PQerrorMessage(m_connection))
            : QStringLiteral("PQconnectdb returned null");
        throw std::runtime_error(QString("PostgreSQL connection failed: %1")
                                     .arg(message.trimmed())
                                     .toStdString());
    }
}

PGconn* DatabaseManager::connection() const
{
    if (m_connection == nullptr || PQstatus(m_connection) != CONNECTION_OK) {
        throw std::runtime_error("database connection is not open");
    }
    return m_connection;
}

void DatabaseManager::ensureSchema()
{
    const QStringList statements = schemaSql().split(QRegularExpression(QStringLiteral(";\\s*\\n")),
                                                     Qt::SkipEmptyParts);
    for (const QString& statement : statements) {
        const QString trimmed = statement.trimmed();
        if (trimmed.isEmpty()) {
            continue;
        }
        exec(trimmed);
    }
}

DatabaseManager::Result::Result(PGresult* result)
    : m_result(result)
{
}

DatabaseManager::Result::~Result()
{
    if (m_result != nullptr) {
        PQclear(m_result);
    }
}

DatabaseManager::Result::Result(Result&& other) noexcept
    : m_result(other.m_result)
{
    other.m_result = nullptr;
}

DatabaseManager::Result& DatabaseManager::Result::operator=(Result&& other) noexcept
{
    if (this != &other) {
        if (m_result != nullptr) {
            PQclear(m_result);
        }
        m_result = other.m_result;
        other.m_result = nullptr;
    }
    return *this;
}

int DatabaseManager::Result::rows() const
{
    return m_result == nullptr ? 0 : PQntuples(m_result);
}

bool DatabaseManager::Result::empty() const
{
    return rows() == 0;
}

QString DatabaseManager::Result::value(int row, int column) const
{
    if (m_result == nullptr || PQgetisnull(m_result, row, column)) {
        return {};
    }
    return QString::fromUtf8(PQgetvalue(m_result, row, column));
}

QByteArray DatabaseManager::Result::bytea(int row, int column) const
{
    if (m_result == nullptr || PQgetisnull(m_result, row, column)) {
        return {};
    }

    size_t size = 0;
    unsigned char* data = PQunescapeBytea(
        reinterpret_cast<const unsigned char*>(PQgetvalue(m_result, row, column)),
        &size);
    if (data == nullptr) {
        throw std::runtime_error("cannot decode PostgreSQL bytea value");
    }
    QByteArray bytes(reinterpret_cast<const char*>(data), static_cast<qsizetype>(size));
    PQfreemem(data);
    return bytes;
}

qint64 DatabaseManager::Result::int64(int row, int column) const
{
    bool ok = false;
    const qint64 parsed = value(row, column).toLongLong(&ok);
    if (!ok) {
        throw std::runtime_error("cannot parse PostgreSQL integer value");
    }
    return parsed;
}

QDateTime DatabaseManager::Result::timestamp(int row, int column) const
{
    return QDateTime::fromString(value(row, column), Qt::ISODate);
}

PGresult* DatabaseManager::Result::raw() const
{
    return m_result;
}

DatabaseManager::Param DatabaseManager::Param::text(const QString& value)
{
    return {value.toUtf8(), 0, 0};
}

DatabaseManager::Param DatabaseManager::Param::int64(qint64 value)
{
    return {QByteArray::number(value), 20, 0};
}

DatabaseManager::Param DatabaseManager::Param::bytea(const QByteArray& value)
{
    return {value, 17, 1};
}

DatabaseManager::Result DatabaseManager::exec(const QString& sql) const
{
    PGresult* result = PQexec(connection(), sql.toUtf8().constData());
    checkResult(result, sql);
    return Result(result);
}

DatabaseManager::Result DatabaseManager::execParams(const QString& sql, const QList<Param>& params) const
{
    std::vector<QByteArray> values;
    std::vector<Oid> types;
    std::vector<int> lengths;
    std::vector<int> formats;
    values.reserve(params.size());
    types.reserve(params.size());
    lengths.reserve(params.size());
    formats.reserve(params.size());

    for (const Param& param : params) {
        values.push_back(param.value);
        types.push_back(param.type);
        lengths.push_back(param.value.size());
        formats.push_back(param.format);
    }

    std::vector<const char*> valuePointers;
    valuePointers.reserve(values.size());
    for (const QByteArray& value : values) {
        valuePointers.push_back(value.constData());
    }

    PGresult* result = PQexecParams(connection(),
                                    sql.toUtf8().constData(),
                                    params.size(),
                                    types.data(),
                                    valuePointers.data(),
                                    lengths.data(),
                                    formats.data(),
                                    0);
    checkResult(result, sql);
    return Result(result);
}

QString DatabaseManager::conninfoQuote(const QString& value)
{
    QString escaped = value;
    escaped.replace(QStringLiteral("\\"), QStringLiteral("\\\\"));
    escaped.replace(QStringLiteral("'"), QStringLiteral("\\'"));
    return QStringLiteral("'") + escaped + QStringLiteral("'");
}

void DatabaseManager::checkResult(PGresult* result, const QString& context) const
{
    if (result == nullptr) {
        throw std::runtime_error(QString("PostgreSQL command failed: %1")
                                     .arg(QString::fromUtf8(PQerrorMessage(connection())).trimmed())
                                     .toStdString());
    }

    const ExecStatusType status = PQresultStatus(result);
    if (status != PGRES_COMMAND_OK && status != PGRES_TUPLES_OK) {
        const QString message = QString::fromUtf8(PQresultErrorMessage(result)).trimmed();
        PQclear(result);
        throw std::runtime_error(QString("PostgreSQL command failed: %1\nSQL: %2")
                                     .arg(message, context)
                                     .toStdString());
    }
}

QString DatabaseManager::schemaSql()
{
    return QStringLiteral(R"SQL(
CREATE TABLE IF NOT EXISTS app_settings (
    key TEXT PRIMARY KEY,
    value TEXT NOT NULL
);

CREATE TABLE IF NOT EXISTS categories (
    id BIGSERIAL PRIMARY KEY,
    name TEXT NOT NULL UNIQUE,
    created_at TIMESTAMPTZ NOT NULL DEFAULT now(),
    updated_at TIMESTAMPTZ NOT NULL DEFAULT now()
);

CREATE TABLE IF NOT EXISTS password_entries (
    id BIGSERIAL PRIMARY KEY,
    category_id BIGINT REFERENCES categories(id) ON DELETE SET NULL,
    title TEXT NOT NULL,

    username_nonce BYTEA NOT NULL,
    username_ciphertext BYTEA NOT NULL,
    username_tag BYTEA NOT NULL,

    password_nonce BYTEA NOT NULL,
    password_ciphertext BYTEA NOT NULL,
    password_tag BYTEA NOT NULL,

    url_nonce BYTEA NOT NULL,
    url_ciphertext BYTEA NOT NULL,
    url_tag BYTEA NOT NULL,

    notes_nonce BYTEA NOT NULL,
    notes_ciphertext BYTEA NOT NULL,
    notes_tag BYTEA NOT NULL,

    created_at TIMESTAMPTZ NOT NULL DEFAULT now(),
    updated_at TIMESTAMPTZ NOT NULL DEFAULT now()
);

CREATE INDEX IF NOT EXISTS idx_password_entries_title ON password_entries USING btree (lower(title));
CREATE INDEX IF NOT EXISTS idx_password_entries_category_id ON password_entries(category_id);
)SQL");
}

} // namespace database
