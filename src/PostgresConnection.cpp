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
    const char* envHost = std::getenv("DB_HOST");
    const char* envPort = std::getenv("DB_PORT");
    const char* envName = std::getenv("DB_NAME");
    const char* envUser = std::getenv("DB_USER");
    const char* envPass = std::getenv("DB_PASSWORD");

    if (envHost) cfg.host = envHost;
    if (envPort) cfg.port = envPort;
    if (envName) cfg.dbname = envName;
    if (envUser) cfg.user = envUser;
    if (envPass) cfg.password = envPass;

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
                    if (key == "DB_HOST" && !envHost) cfg.host = val;
                    else if (key == "DB_PORT" && !envPort) cfg.port = val;
                    else if (key == "DB_NAME" && !envName) cfg.dbname = val;
                    else if (key == "DB_USER" && !envUser) cfg.user = val;
                    else if (key == "DB_PASSWORD" && !envPass) cfg.password = val;
                }
            }
        }
    }

    return cfg;
}

PostgresConnection::PostgresConnection(const DbConfig& cfg) : config(cfg) {
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

pqxx::connection& PostgresConnection::getRawConnection() {
    if (!connection) {
        throw AllocationException::DatabaseException("Underlying connection is null.");
    }
    return *connection;
}
