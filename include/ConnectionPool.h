#pragma once

#include <memory>
#include <vector>
#include "PostgresConnection.h"

class ConnectionPool;

// RAII lease on a pooled connection. While a PooledConnection is alive the
// caller owns the underlying PostgresConnection; when it goes out of scope the
// connection is automatically returned to the pool for reuse (no manual
// release call, same RAII idea as PostgresConnection itself).
class PooledConnection {
private:
    ConnectionPool* pool;
    std::unique_ptr<PostgresConnection> conn;

public:
    PooledConnection(ConnectionPool* owningPool, std::unique_ptr<PostgresConnection> connection);
    ~PooledConnection();

    // Move-only: a lease represents exclusive temporary ownership.
    PooledConnection(PooledConnection&& other) noexcept;
    PooledConnection& operator=(PooledConnection&& other) noexcept;
    PooledConnection(const PooledConnection&) = delete;
    PooledConnection& operator=(const PooledConnection&) = delete;

    PostgresConnection& get();
    PostgresConnection* operator->();
    PostgresConnection& operator*();
};

// A small pool of reusable PostgreSQL connections.
//
// Opening a TCP connection per query is wasteful, so the pool keeps a few idle
// connections and hands them out through acquire(). This project is single
// threaded, so the pool needs no locking; it simply avoids repeated connect
// handshakes during a burst of queries.
class ConnectionPool {
private:
    DbConfig config;
    size_t maxIdle;
    std::vector<std::unique_ptr<PostgresConnection>> idle;

public:
    explicit ConnectionPool(const DbConfig& config, size_t maxIdle = 4);

    // Returns a lease wrapping an idle connection, or a freshly opened one if
    // none are idle.
    PooledConnection acquire();

    // Takes a connection back from a lease. Kept for reuse if still open and
    // there is room; otherwise allowed to close. Called by PooledConnection.
    void release(std::unique_ptr<PostgresConnection> connection);

    // Number of connections currently idle in the pool (for tests/inspection).
    size_t idleCount() const;
};
