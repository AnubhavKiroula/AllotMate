// ============================================================
// test_db_layer.cpp -- repository / DB integration tests
//
// These need a live PostgreSQL seeded with sql/01+02. If no database is
// reachable the test prints [SKIP] and exits 0, so `ctest` stays green on
// machines (and CI jobs) without a database.
//
// Seed data is protected two ways:
//   * the rollback test writes inside an uncommitted transaction;
//   * every committed write uses a sentinel roll number and is deleted again.
// ============================================================

#include <iostream>
#include <string>
#include <vector>

#include "PostgresConnection.h"
#include "PostgresStudentRepo.h"
#include "PostgresElectiveRepo.h"
#include "RunLogRepository.h"
#include "ConnectionPool.h"
#include "StudentRecord.h"
#include "AllocationException.h"

static int g_failures = 0;

#define check(cond, msg)                              \
    do {                                              \
        if (!(cond)) {                                \
            std::cout << "  [FAIL] " << (msg) << "\n"; \
            g_failures++;                             \
        } else {                                      \
            std::cout << "  [ok]   " << (msg) << "\n"; \
        }                                             \
    } while (0)

static void testRollbackProtectsSeedData(PostgresConnection& conn) {
    std::cout << "testRollbackProtectsSeedData\n";
    const std::string roll = "ROLLBACKTEST01";

    {
        pqxx::work txn(conn.getRawConnection());
        txn.exec("INSERT INTO STUDENT (name, roll_no, cgpa, branch, status) VALUES "
                 "('Rollback Probe', '" + roll + "', 7.00, 'TEST', 'UNASSIGNED');");
        pqxx::result inside =
            txn.exec("SELECT count(*) FROM STUDENT WHERE roll_no = '" + roll + "';");
        check(inside[0][0].as<int>() == 1, "row is visible inside the transaction");
        // txn destructs here with no commit() -> PostgreSQL rolls it back.
    }

    pqxx::result after =
        conn.executeQuery("SELECT count(*) FROM STUDENT WHERE roll_no = '" + roll + "';");
    check(after[0][0].as<int>() == 0, "row is gone after rollback (seed data untouched)");
}

static void testElectiveRepo(PostgresConnection& conn) {
    std::cout << "testElectiveRepo\n";
    PostgresElectiveRepo repo(conn);

    std::vector<Elective> all = repo.findAll();
    check(!all.empty(), "findAll returns seeded electives");

    int probeId = all.front().getId();
    Elective one = repo.findById(probeId);
    check(one.getId() == probeId, "findById returns the requested elective");

    // Round-trip save: bump capacity, confirm it persisted, then restore.
    int originalCapacity = one.getCapacity();
    Elective bumped(probeId, one.getName(), originalCapacity + 5);
    repo.save(bumped);
    check(repo.findById(probeId).getCapacity() == originalCapacity + 5,
          "save persists an updated capacity");

    Elective restored(probeId, one.getName(), originalCapacity);
    repo.save(restored);
    check(repo.findById(probeId).getCapacity() == originalCapacity,
          "capacity restored to its seeded value");
}

static void testStudentRepoAndBulkInsert(PostgresConnection& conn) {
    std::cout << "testStudentRepoAndBulkInsert\n";
    PostgresStudentRepo repo(conn);

    std::vector<Student> all = repo.findAll();
    check(!all.empty(), "findAll returns seeded students");
    check(!all.front().getPreferenceList().empty(),
          "students load with their ranked preferences");

    // The merge-join must attach EVERY preference row, not just the first few.
    size_t loadedPrefs = 0;
    for (const Student& s : all) {
        loadedPrefs += s.getPreferenceList().size();
    }
    pqxx::result prefCount = conn.executeQuery("SELECT count(*) FROM PREFERENCE;");
    check(loadedPrefs == static_cast<size_t>(prefCount[0][0].as<int>()),
          "findAll attaches every preference row (merge-join is complete)");

    // Clean any leftovers from a previous failed run, then bulk-insert sentinels.
    conn.executeCommand("DELETE FROM STUDENT WHERE roll_no LIKE 'ITEST%';");

    std::vector<StudentRecord> records = {
        { "ITEST0001", "Import One",   8.10, "CSE" },
        { "ITEST0002", "Import Two",   7.40, "IT" },
        { "ITEST0003", "Import Three", 9.00, "ECE" },
    };
    int inserted = repo.bulkInsert(records);
    check(inserted == 3, "bulkInsert reports three new rows");

    int insertedAgain = repo.bulkInsert(records);
    check(insertedAgain == 0, "re-inserting the same roll numbers inserts nothing (dedup)");

    conn.executeCommand("DELETE FROM STUDENT WHERE roll_no LIKE 'ITEST%';");
    pqxx::result left =
        conn.executeQuery("SELECT count(*) FROM STUDENT WHERE roll_no LIKE 'ITEST%';");
    check(left[0][0].as<int>() == 0, "sentinel rows cleaned up");
}

static void testRunLog(PostgresConnection& conn) {
    std::cout << "testRunLog\n";
    RunLogRepository repo(conn);

    int runId = repo.logRun(-1, 0, "unit-test run: 0 blocking pairs");
    check(runId > 0, "logRun returns a generated run id");

    std::vector<RunLogEntry> recent = repo.recentRuns(5);
    bool found = false;
    for (const auto& entry : recent) {
        if (entry.runId == runId) {
            found = true;
            check(entry.blockingPairsFound == 0, "logged blocking-pair count read back correctly");
        }
    }
    check(found, "logged run appears in recentRuns");

    // A run whose stability check was not performed is stored as NULL and
    // read back as -1 -- never as a misleading 0.
    int uncheckedId = repo.logRun(-1, -1, "unit-test run: stability not checked");
    pqxx::result stored = conn.executeQuery(
        "SELECT blocking_pairs_found IS NULL AS is_null FROM RUN_LOG WHERE run_id = "
        + std::to_string(uncheckedId) + ";");
    check(stored[0]["is_null"].as<bool>(), "unchecked run stores NULL blocking pairs");

    bool readBackAsMinusOne = false;
    for (const auto& entry : repo.recentRuns(5)) {
        if (entry.runId == uncheckedId) {
            readBackAsMinusOne = (entry.blockingPairsFound == -1);
        }
    }
    check(readBackAsMinusOne, "NULL blocking pairs read back as -1 (not computed)");

    conn.executeCommand("DELETE FROM RUN_LOG WHERE run_id IN (" + std::to_string(runId)
                        + ", " + std::to_string(uncheckedId) + ");");
}

static void testConnectionPoolReuse(const DbConfig& cfg) {
    std::cout << "testConnectionPoolReuse\n";
    ConnectionPool pool(cfg, 2);
    check(pool.idleCount() == 0, "pool starts empty");

    {
        PooledConnection lease = pool.acquire();
        pqxx::result r = lease->executeQuery("SELECT 1 AS ok;");
        check(r[0]["ok"].as<int>() == 1, "query runs through a leased connection");
    } // lease returns the connection to the pool here

    check(pool.idleCount() == 1, "connection returned to pool on lease destruction");

    PooledConnection reused = pool.acquire();
    check(pool.idleCount() == 0, "second acquire reuses the idle connection");
}

int main() {
    std::cout << "=== AllotMate DB-layer integration tests ===\n";

    DbConfig cfg = DbConfig::fromEnv();
    try {
        PostgresConnection probe(cfg);
        if (!probe.isConnected()) {
            throw AllocationException::DatabaseException("connection not open");
        }
    } catch (const std::exception& e) {
        std::cout << "  [SKIP] no database reachable (" << e.what() << ")\n";
        std::cout << "  Set DB_* env vars / run `docker compose up -d` to execute these tests.\n";
        return 0;
    }

    PostgresConnection conn(cfg);
    testRollbackProtectsSeedData(conn);
    testElectiveRepo(conn);
    testStudentRepoAndBulkInsert(conn);
    testRunLog(conn);
    testConnectionPoolReuse(cfg);

    std::cout << "============================================\n";
    if (g_failures == 0) {
        std::cout << "All DB-layer checks passed.\n";
        return 0;
    }
    std::cout << g_failures << " check(s) failed.\n";
    return 1;
}
