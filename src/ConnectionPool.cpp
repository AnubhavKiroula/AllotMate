#include "ConnectionPool.h"
#include "AllocationException.h"

#include <utility>

// ---- PooledConnection ----

PooledConnection::PooledConnection(ConnectionPool* owningPool,
                                   std::unique_ptr<PostgresConnection> connection)
    : pool(owningPool), conn(std::move(connection)) {}

PooledConnection::~PooledConnection() {
    if (pool && conn) {
        pool->release(std::move(conn));
    }
}

PooledConnection::PooledConnection(PooledConnection&& other) noexcept
    : pool(other.pool), conn(std::move(other.conn)) {
    other.pool = nullptr; // moved-from lease must not release on destruction
}

PooledConnection& PooledConnection::operator=(PooledConnection&& other) noexcept {
    if (this != &other) {
        if (pool && conn) {
            pool->release(std::move(conn));
        }
        pool = other.pool;
        conn = std::move(other.conn);
        other.pool = nullptr;
    }
    return *this;
}

PostgresConnection& PooledConnection::get() {
    if (!conn) {
        throw AllocationException::DatabaseException("PooledConnection holds no connection.");
    }
    return *conn;
}

PostgresConnection* PooledConnection::operator->() { return &get(); }
PostgresConnection& PooledConnection::operator*() { return get(); }

// ---- ConnectionPool ----

ConnectionPool::ConnectionPool(const DbConfig& cfg, size_t maxIdleConnections)
    : config(cfg), maxIdle(maxIdleConnections) {}

PooledConnection ConnectionPool::acquire() {
    if (!idle.empty()) {
        std::unique_ptr<PostgresConnection> conn = std::move(idle.back());
        idle.pop_back();
        return PooledConnection(this, std::move(conn));
    }
    // No idle connection available: open a new one.
    return PooledConnection(this, std::make_unique<PostgresConnection>(config));
}

void ConnectionPool::release(std::unique_ptr<PostgresConnection> connection) {
    if (connection && connection->isConnected() && idle.size() < maxIdle) {
        idle.push_back(std::move(connection));
    }
    // Otherwise the connection is simply destroyed here (RAII closes it).
}

size_t ConnectionPool::idleCount() const {
    return idle.size();
}
