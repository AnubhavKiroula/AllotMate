#pragma once

#include <string>
#include <memory>
#include <pqxx/pqxx>
#include "AllocationException.h"

struct DbConfig {
    std::string host = "localhost";
    std::string port = "5432";
    std::string dbname = "allotmate";
    std::string user = "postgres";
    std::string password = "postgres";

    std::string toConnectionString() const;
    static DbConfig fromEnv(const std::string& envFilePath = ".env");
};

class PostgresConnection {
private:
    std::unique_ptr<pqxx::connection> connection;
    DbConfig config;

public:
    // Opens connection using loaded DbConfig; throws DatabaseException on failure
    explicit PostgresConnection(const DbConfig& cfg = DbConfig::fromEnv());
    
    // RAII destructor cleanly terminates connection
    ~PostgresConnection();

    // Non-copyable (unique resource ownership)
    PostgresConnection(const PostgresConnection&) = delete;
    PostgresConnection& operator=(const PostgresConnection&) = delete;

    // Movable
    PostgresConnection(PostgresConnection&&) noexcept = default;
    PostgresConnection& operator=(PostgresConnection&&) noexcept = default;

    // Executes a query (SELECT) and returns result
    pqxx::result executeQuery(const std::string& sql);

    // Executes a command (INSERT, UPDATE, DELETE) and returns affected row count
    int executeCommand(const std::string& sql);

    // Checks if connection is active and open
    bool isConnected() const;

    // Access raw underlying pqxx connection reference
    pqxx::connection& getRawConnection();
};
