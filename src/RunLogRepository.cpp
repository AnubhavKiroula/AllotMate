#include "RunLogRepository.h"
#include "AllocationException.h"

#include <sstream>

RunLogRepository::RunLogRepository(PostgresConnection& connection)
    : db(connection) {}

int RunLogRepository::logRun(int coordinatorId, int blockingPairsFound,
                             const std::string& summary) {
    std::ostringstream sql;
    sql << "INSERT INTO RUN_LOG (coordinator_id, blocking_pairs_found, outcome_summary) VALUES (";
    if (coordinatorId > 0) {
        sql << coordinatorId;
    } else {
        sql << "NULL"; // run not attributed to a specific coordinator
    }
    sql << ", " << blockingPairsFound
        << ", " << db.quoteLiteral(summary)
        << ") RETURNING run_id;";

    pqxx::result res = db.executeQuery(sql.str());
    if (res.empty()) {
        throw AllocationException::DatabaseException("Failed to write RUN_LOG entry.");
    }
    return res[0]["run_id"].as<int>();
}

std::vector<RunLogEntry> RunLogRepository::recentRuns(int limit) {
    if (limit <= 0) {
        limit = 10;
    }

    std::ostringstream sql;
    sql << "SELECT run_id, coordinator_id, run_at, blocking_pairs_found, outcome_summary "
        << "FROM RUN_LOG ORDER BY run_at DESC, run_id DESC LIMIT " << limit << ";";
    pqxx::result rows = db.executeQuery(sql.str());

    std::vector<RunLogEntry> entries;
    entries.reserve(rows.size());
    for (const auto& row : rows) {
        RunLogEntry entry;
        entry.runId = row["run_id"].as<int>();
        // coordinator_id is nullable; fall back to -1 when NULL.
        entry.coordinatorId = row["coordinator_id"].is_null()
                                  ? -1
                                  : row["coordinator_id"].as<int>();
        entry.runAt = row["run_at"].as<std::string>();
        entry.blockingPairsFound = row["blocking_pairs_found"].as<int>();
        entry.outcomeSummary = row["outcome_summary"].is_null()
                                   ? ""
                                   : row["outcome_summary"].as<std::string>();
        entries.push_back(entry);
    }
    return entries;
}
