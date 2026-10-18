#include "PostgresConnection.h"
#include <fstream>
#include <sstream>
#include <cstdlib>
#include <algorithm>

namespace {
    std::string trim(const std::string& str) {
        size_t first = str.find_first_not_of(" \t\r\n");
        if (first == std::string::npos) return "";
        size_t last = str.find_last_not_of(" \t\r\n");
        std::string res = str.substr(first, (last - first + 1));
        if (res.size() >= 2 && ((res.front() == '"' && res.back() == '"') || (res.front() == '\'' && res.back() == '\''))) {
            res = res.substr(1, res.size() - 2);
        }
        return res;
    }
    std::string getEnvironmentVariable(const char* name) {
    #ifdef _WIN32
        char* value = nullptr;
        size_t size = 0;

        if (_dupenv_s(&value, &size, name) != 0 || value == nullptr) {
            return {};
        }

        std::string result(value);
        free(value);
        return result;
    #else
        const char* value = std::getenv(name);
        return value ? value : "";
    #endif
    }
}

std::string DbConfig::toConnectionString() const {
    std::ostringstream oss;
    oss << "host=" << host
        << " port=" << port
        << " dbname=" << dbname
        << " user=" << user
        << " password=" << password;
    return oss.str();
}

DbConfig DbConfig::fromEnv(const std::string& envFilePath) {
    DbConfig cfg;

    // Check system environment variables first
    const std::string envHost = getEnvironmentVariable("DB_HOST");
    const std::string envPort = getEnvironmentVariable("DB_PORT");
    const std::string envName = getEnvironmentVariable("DB_NAME");
    const std::string envUser = getEnvironmentVariable("DB_USER");
    const std::string envPass = getEnvironmentVariable("DB_PASSWORD");

    if (!envHost.empty()) cfg.host = envHost;
    if (!envPort.empty()) cfg.port = envPort;
    if (!envName.empty()) cfg.dbname = envName;
    if (!envUser.empty()) cfg.user = envUser;
    if (!envPass.empty()) cfg.password = envPass;

    // Read .env file if available to fill in any unset fields
    std::ifstream file(envFilePath);
    if (file.is_open()) {
        std::string line;
        while (std::getline(file, line)) {
            line = trim(line);
            if (line.empty() || line[0] == '#') continue;
            size_t eqPos = line.find('=');
            if (eqPos != std::string::npos) {
                std::string key = trim(line.substr(0, eqPos));
                std::string val = trim(line.substr(eqPos + 1));
                if (!val.empty()) {
                    if (key == "DB_HOST" && envHost.empty()) cfg.host = val;
                    else if (key == "DB_PORT" && envPort.empty()) cfg.port = val;
                    else if (key == "DB_NAME" && envName.empty()) cfg.dbname = val;
                    else if (key == "DB_USER" && envUser.empty()) cfg.user = val;
                    else if (key == "DB_PASSWORD" && envPass.empty()) cfg.password = val;
                }
            }
        }
    }

    return cfg;
}

PostgresConnection::PostgresConnection(const DbConfig& cfg) : connection(nullptr), config(cfg) {
    try {
        std::string connStr = config.toConnectionString();
        connection = std::make_unique<pqxx::connection>(connStr);
        if (!connection->is_open()) {
            throw AllocationException::DatabaseException("Failed to open connection to PostgreSQL server.");
        }
    } catch (const std::exception& e) {
        throw AllocationException::DatabaseException(std::string("PostgreSQL connection error: ") + e.what());
    }
}

PostgresConnection::~PostgresConnection() {
    try {
        if (connection && connection->is_open()) {
            connection->close();
        }
    } catch (...) {
        // Suppress destructors from throwing
    }
}

pqxx::result PostgresConnection::executeQuery(const std::string& sql) {
    if (!isConnected()) {
        throw AllocationException::DatabaseException("Cannot execute query: PostgreSQL connection is not open.");
    }
    try {
        pqxx::work txn(*connection);
        pqxx::result res = txn.exec(sql);
        txn.commit();
        return res;
    } catch (const std::exception& e) {
        throw AllocationException::DatabaseException(std::string("Query execution failed: ") + e.what());
    }
}

int PostgresConnection::executeCommand(const std::string& sql) {
    if (!isConnected()) {
        throw AllocationException::DatabaseException("Cannot execute command: PostgreSQL connection is not open.");
    }
    try {
        pqxx::work txn(*connection);
        pqxx::result res = txn.exec(sql);
        txn.commit();
        return static_cast<int>(res.affected_rows());
    } catch (const std::exception& e) {
        throw AllocationException::DatabaseException(std::string("Command execution failed: ") + e.what());
    }
}

bool PostgresConnection::isConnected() const {
    return connection && connection->is_open();
}

std::string PostgresConnection::quoteLiteral(const std::string& value) const {
    if (!connection) {
        throw AllocationException::DatabaseException("Cannot quote value: connection is null.");
    }
    // connection->esc() doubles any embedded single quotes; we wrap the result
    // in quotes ourselves so the output can be dropped straight into SQL.
    return "'" + connection->esc(value) + "'";
}

pqxx::connection& PostgresConnection::getRawConnection() {
    if (!connection) {
        throw AllocationException::DatabaseException("Underlying connection is null.");
    }
    return *connection;
}
