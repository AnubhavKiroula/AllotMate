#pragma once

#include <string>
#include <vector>
#include "PostgresConnection.h"

// One immutable audit record of a Gale-Shapley matching run.
struct RunLogEntry {
    int runId;
    int coordinatorId;        // -1 when the run was not tied to a coordinator
    std::string runAt;        // timestamp as returned by PostgreSQL
    int blockingPairsFound;
    std::string outcomeSummary;
};

// Writes and reads the RUN_LOG audit table. This is intentionally not a
// Repository<T>: RUN_LOG records an event, so it needs an append + recent-reads
// API rather than the entity CRUD shape of Repository.
class RunLogRepository {
private:
    PostgresConnection& db;

public:
    explicit RunLogRepository(PostgresConnection& connection);

    // Appends an audit row and returns the generated run_id. Pass a
    // coordinatorId <= 0 to store NULL (run not attributed to a coordinator).
    int logRun(int coordinatorId, int blockingPairsFound, const std::string& summary);

    // Returns the most recent runs, newest first (for the audit/report screen).
    std::vector<RunLogEntry> recentRuns(int limit);
};
