#pragma once

#include <string>
#include "PostgresConnection.h"
#include "MatchingEngine.h"

// What one allocation run produced (returned to the CLI for display).
struct AllocationSummary {
    int totalStudents;
    int matched;
    int unmatched;
    int runId; // RUN_LOG id of this run
};

// RAII guard for the database-level "one allocation at a time" lock.
//
// It uses a PostgreSQL advisory lock: every run asks for the same lock number,
// and PostgreSQL grants it to only one connection at a time. The lock is taken
// in the constructor and always released in the destructor -- even if the run
// throws halfway -- so a crashed run can never leave the system locked.
class RunLock {
private:
    PostgresConnection& db;

public:
    // Throws AllocationException if another connection already holds the lock.
    explicit RunLock(PostgresConnection& connection);
    ~RunLock();

    RunLock(const RunLock&) = delete;
    RunLock& operator=(const RunLock&) = delete;
};

// Runs a complete allocation: database -> MatchingEngine -> database.
//
//   1. lock      : refuse to start if another run is in progress (SRS #10)
//   2. re-run    : refuse to overwrite an existing round unless the coordinator
//                  explicitly allows it (SRS #7 -- never silently overwrite)
//   3. load      : students + electives through the repositories
//   4. match     : Gale-Shapley via MatchingEngine (Member 1's module)
//   5. save      : allotments, statuses and seat counts in ONE transaction
//   6. audit     : append a RUN_LOG row describing the outcome
class AllocationService {
private:
    PostgresConnection& db;

    bool roundAlreadyAllocated(const std::string& round);

    // Writes the engine's results for `round` atomically; returns matched count.
    int saveResults(const MatchingEngine& engine, const std::string& round);

public:
    // Any fixed number works; all runs just have to agree on it.
    static constexpr long long RUN_LOCK_KEY = 7310;

    explicit AllocationService(PostgresConnection& connection);

    // coordinatorId <= 0 records the run without a coordinator.
    // Throws AlreadyAllottedException if `round` already has allotments and
    // allowRerun is false; throws AllocationException if another run is active.
    AllocationSummary runAllocation(int coordinatorId, const std::string& round, bool allowRerun);
};
