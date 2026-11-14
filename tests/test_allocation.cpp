// ============================================================
// test_allocation.cpp -- end-to-end allocation tests
//
// Runs the real pipeline (DB -> MatchingEngine -> DB) on the seeded data and
// checks the PROPERTIES the result must have, reading everything back from
// PostgreSQL:
//   * capacity    : no elective holds more students than its capacity
//   * uniqueness  : a student has at most one allotment per round
//   * validity    : a student is only placed in an elective they ranked
//   * stability   : no blocking pair exists in the saved result
//   * waitlists   : students wait exactly for the electives they ranked above
//                   their result, in AVL priority order
//   * re-run rule : an allocated round is never silently overwritten
//   * locking     : a second concurrent run is refused
//   * determinism : re-running gives the same allotments
//
// Skips (exit 0) when no database is reachable, or when the database already
// holds real allotments -- this test resets statuses/seat counts afterwards,
// so it must only ever run against seed data.
// ============================================================

#include <iostream>
#include <map>
#include <set>
#include <string>
#include <vector>

#include "PostgresConnection.h"
#include "PostgresStudentRepo.h"
#include "PostgresElectiveRepo.h"
#include "AllocationService.h"
#include "AllocationException.h"
#include "ReportRepository.h"

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

static const std::string ROUND = "TEST-ROUND";

// student_id -> elective_id for every allotment of a round.
static std::map<int, int> loadAllotments(PostgresConnection& conn, const std::string& round) {
    std::map<int, int> result;
    pqxx::result rows = conn.executeQuery(
        "SELECT student_id, elective_id FROM ALLOTMENT WHERE round = '" + round + "';");
    for (const auto& row : rows) {
        result[row["student_id"].as<int>()] = row["elective_id"].as<int>();
    }
    return result;
}

// Position of an elective in a student's preference list (0 = first choice),
// or -1 if the student never ranked it.
static int rankOf(const Student& s, int electiveId) {
    const std::vector<int>& prefs = s.getPreferenceList();
    for (size_t i = 0; i < prefs.size(); i++) {
        if (prefs[i] == electiveId) {
            return static_cast<int>(i);
        }
    }
    return -1;
}

// Stability oracle (test-side, independent of the engine), computed from what
// is saved in the database: for every student s and every elective e that s
// ranks ABOVE its current placement, e must be full and every student in e must
// outrank s. Otherwise (s, e) is a blocking pair. Returns how many exist.
static int countBlockingPairs(PostgresConnection& conn, const std::string& round) {
    std::map<int, int> allotted = loadAllotments(conn, round);
    std::vector<Student> students = PostgresStudentRepo(conn).findAll();
    std::vector<Elective> electives = PostgresElectiveRepo(conn).findAll();

    std::map<int, const Student*> byId;
    for (const Student& s : students) {
        byId[s.getId()] = &s;
    }
    std::map<int, std::vector<const Student*>> occupants;
    for (const auto& pair : allotted) {
        occupants[pair.second].push_back(byId[pair.first]);
    }
    std::map<int, int> capacityOf;
    for (const Elective& e : electives) {
        capacityOf[e.getId()] = e.getCapacity();
    }

    int blockingPairs = 0;
    for (const Student& s : students) {
        auto placed = allotted.find(s.getId());
        int placedRank = (placed == allotted.end())
                             ? static_cast<int>(s.getPreferenceList().size())
                             : rankOf(s, placed->second);

        for (int r = 0; r < placedRank; r++) {
            int e = s.getPreferenceList()[r];
            const std::vector<const Student*>& inside = occupants[e];
            bool hasFreeSeat = static_cast<int>(inside.size()) < capacityOf[e];
            bool someoneWeaker = false;
            for (const Student* t : inside) {
                if (!(s < *t)) someoneWeaker = true; // e would rather have s than t
            }
            if (hasFreeSeat || someoneWeaker) blockingPairs++;
        }
    }
    return blockingPairs;
}

static void restoreSeedState(PostgresConnection& conn) {
    conn.executeCommand("DELETE FROM ALLOTMENT WHERE round LIKE 'TEST-ROUND%';");
    conn.executeCommand("UPDATE STUDENT SET status = 'UNASSIGNED';");
    conn.executeCommand("UPDATE ELECTIVE SET seats_filled = 0;");
    conn.executeCommand("DELETE FROM RUN_LOG WHERE outcome_summary LIKE 'TEST-ROUND%';");
    conn.executeCommand("DELETE FROM WAITLIST;"); // the seed has no waitlist rows
}

// First column of the first row of a COUNT(*) query.
static int countOf(PostgresConnection& conn, const std::string& sql) {
    return conn.executeQuery(sql)[0][0].as<int>();
}

static void testRunProducesValidStableResult(PostgresConnection& conn) {
    std::cout << "testRunProducesValidStableResult\n";
    AllocationService service(conn);
    AllocationSummary summary = service.runAllocation(-1, ROUND, false);

    std::map<int, int> allotted = loadAllotments(conn, ROUND);
    std::vector<Student> students = PostgresStudentRepo(conn).findAll();
    std::vector<Elective> electives = PostgresElectiveRepo(conn).findAll();

    check(summary.matched == static_cast<int>(allotted.size()),
          "summary.matched equals the allotment rows saved");
    check(summary.matched + summary.unmatched == summary.totalStudents,
          "matched + unmatched accounts for every student");

    // Capacity: allotments per elective <= capacity, and seats_filled agrees.
    std::map<int, int> countPerElective;
    for (const auto& pair : allotted) {
        countPerElective[pair.second]++;
    }
    bool capacityOk = true;
    bool seatsAgree = true;
    for (const Elective& e : electives) {
        if (countPerElective[e.getId()] > e.getCapacity()) capacityOk = false;
        if (countPerElective[e.getId()] != e.getSeatsFilled()) seatsAgree = false;
    }
    check(capacityOk, "no elective is over capacity");
    check(seatsAgree, "seats_filled matches the saved allotments");

    // Validity: everyone placed in an elective they actually ranked.
    std::map<int, const Student*> byId;
    for (const Student& s : students) {
        byId[s.getId()] = &s;
    }
    bool allRanked = true;
    for (const auto& pair : allotted) {
        if (rankOf(*byId[pair.first], pair.second) == -1) allRanked = false;
    }
    check(allRanked, "every allotment is one of the student's own preferences");

    // Status column agrees with the allotments.
    pqxx::result allottedStatus = conn.executeQuery(
        "SELECT count(*) FROM STUDENT WHERE status = 'ALLOTTED';");
    check(allottedStatus[0][0].as<int>() == summary.matched,
          "exactly the matched students are marked ALLOTTED");

    check(countBlockingPairs(conn, ROUND) == 0,
          "no blocking pairs in the saved result (stable matching)");

    // The engine's fairness report must agree with this independent check.
    check(summary.fairness.blockingPairs == 0 && summary.fairness.matched == summary.matched,
          "fairness report agrees with the saved result (0 blocking pairs)");

    pqxx::result logRow = conn.executeQuery(
        "SELECT blocking_pairs_found, outcome_summary FROM RUN_LOG "
        "WHERE run_id = " + std::to_string(summary.runId) + ";");

    check(logRow.size() == 1, "run is recorded in RUN_LOG");

    check(
        logRow.size() == 1 &&
        !logRow[0]["blocking_pairs_found"].is_null() &&
        logRow[0]["blocking_pairs_found"].as<int>() == 0,
        "RUN_LOG stores the actual blocking-pair count (0 for stable matching)"
    );
}

// Checks the waitlists saved by the run above (FR-7), straight from the DB.
static void testWaitlistsFollowTheResult(PostgresConnection& conn) {
    std::cout << "testWaitlistsFollowTheResult\n";
    const std::string round = conn.quoteLiteral(ROUND);

    // Rows a student should wait for: an elective ranked above their result,
    // or any ranked elective if they got nothing.
    const std::string shouldWait =
        "SELECT count(*) FROM PREFERENCE p "
        "LEFT JOIN ALLOTMENT a ON a.student_id = p.student_id AND a.round = " + round + " "
        "LEFT JOIN PREFERENCE pa ON pa.student_id = a.student_id AND pa.elective_id = a.elective_id "
        "WHERE a.student_id IS NULL OR p.rank < pa.rank;";
    const int expected = countOf(conn, shouldWait);
    check(expected > 0, "the seed data leaves some students waiting (test is not vacuous)");
    check(countOf(conn, "SELECT count(*) FROM WAITLIST;") == expected,
          "every student waits for every elective they ranked above their result");

    check(countOf(conn,
              "SELECT count(*) FROM WAITLIST w "
              "LEFT JOIN PREFERENCE pw ON pw.student_id = w.student_id AND pw.elective_id = w.elective_id "
              "LEFT JOIN ALLOTMENT a ON a.student_id = w.student_id AND a.round = " + round + " "
              "LEFT JOIN PREFERENCE pa ON pa.student_id = a.student_id AND pa.elective_id = a.elective_id "
              "WHERE pw.student_id IS NULL OR (a.student_id IS NOT NULL AND pw.rank >= pa.rank);") == 0,
          "nobody waits for an elective they did not rank above their result");

    check(countOf(conn,
              "SELECT count(*) FROM WAITLIST w1 "
              "JOIN WAITLIST w2 ON w2.elective_id = w1.elective_id AND w1.position < w2.position "
              "JOIN STUDENT s1 ON s1.student_id = w1.student_id "
              "JOIN STUDENT s2 ON s2.student_id = w2.student_id "
              "WHERE s1.cgpa < s2.cgpa OR (s1.cgpa = s2.cgpa AND s1.student_id > s2.student_id);") == 0,
          "each waitlist is in AVL priority order (CGPA, then lower id)");

    // Stability means nobody waiting outranks anyone inside, and every
    // elective with a waitlist is full.
    check(countOf(conn,
              "SELECT count(*) FROM WAITLIST w "
              "JOIN ALLOTMENT a ON a.elective_id = w.elective_id AND a.round = " + round + " "
              "JOIN STUDENT sw ON sw.student_id = w.student_id "
              "JOIN STUDENT sa ON sa.student_id = a.student_id "
              "WHERE sw.cgpa > sa.cgpa OR (sw.cgpa = sa.cgpa AND sw.student_id < sa.student_id);") == 0,
          "everyone allotted to an elective outranks everyone waiting for it");
    check(countOf(conn,
              "SELECT count(*) FROM ELECTIVE e WHERE e.seats_filled < e.capacity "
              "AND EXISTS (SELECT 1 FROM WAITLIST w WHERE w.elective_id = e.elective_id);") == 0,
          "every elective with a waitlist is full");

    check(countOf(conn,
              "SELECT count(*) FROM STUDENT s WHERE (s.status = 'WAITLISTED') <> ("
              "NOT EXISTS (SELECT 1 FROM ALLOTMENT a WHERE a.student_id = s.student_id AND a.round = " + round + ") "
              "AND EXISTS (SELECT 1 FROM WAITLIST w WHERE w.student_id = s.student_id));") == 0,
          "status is WAITLISTED exactly for unmatched students on a waitlist");

    // The student status screen (FR-2) must find all of one student's
    // waitlists, best-ranked choice first.
    pqxx::result busiest = conn.executeQuery(
        "SELECT s.roll_no, count(*) FROM WAITLIST w JOIN STUDENT s ON s.student_id = w.student_id "
        "GROUP BY s.roll_no ORDER BY count(*) DESC, s.roll_no LIMIT 1;");
    std::vector<WaitingForRow> rows =
        ReportRepository(conn).waitlistPositionsOf(busiest[0][0].as<std::string>());
    bool bestChoiceFirst = true;
    for (size_t i = 1; i < rows.size(); i++) {
        if (rows[i].choiceRank <= rows[i - 1].choiceRank) bestChoiceFirst = false;
    }
    check(static_cast<int>(rows.size()) == busiest[0][1].as<int>() && bestChoiceFirst &&
              rows[0].position >= 1 && rows[0].position <= rows[0].waitingTotal,
          "waitlistPositionsOf shows every waitlist of a student, best choice first");
}

// Proves the stability oracle is not vacuous: break a stable result on purpose
// and check that the oracle reports the blocking pair it creates.
static void testOracleDetectsABlockingPair(PostgresConnection& conn) {
    std::cout << "testOracleDetectsABlockingPair\n";

    // Find a student who got their FIRST choice and also ranked another elective.
    std::vector<Student> students = PostgresStudentRepo(conn).findAll();
    std::map<int, int> allotted = loadAllotments(conn, ROUND);
    int victimId = -1;
    int worseElective = -1;
    for (const Student& s : students) {
        auto placed = allotted.find(s.getId());
        if (placed != allotted.end() && rankOf(s, placed->second) == 0
            && s.getPreferenceList().size() > 1) {
            victimId = s.getId();
            worseElective = s.getPreferenceList().back();
            break;
        }
    }
    check(victimId != -1, "found a first-choice student to move");
    if (victimId == -1) return;

    // Move them down to their last choice. Their first choice now has a free
    // seat they would prefer -> (victim, first choice) is a blocking pair.
    conn.executeCommand("UPDATE ALLOTMENT SET elective_id = " + std::to_string(worseElective)
                        + " WHERE round = '" + ROUND + "' AND student_id = "
                        + std::to_string(victimId) + ";");
    check(countBlockingPairs(conn, ROUND) > 0, "oracle detects the planted blocking pair");

    // Repair by re-running the allocation for the round.
    AllocationService(conn).runAllocation(-1, ROUND, true);
    check(countBlockingPairs(conn, ROUND) == 0, "re-running restores a stable result");
}

static void testRerunNeedsExplicitPermission(PostgresConnection& conn) {
    std::cout << "testRerunNeedsExplicitPermission\n";
    AllocationService service(conn);
    std::map<int, int> before = loadAllotments(conn, ROUND);

    bool refused = false;
    try {
        service.runAllocation(-1, ROUND, false);
    } catch (const AllocationException::AlreadyAllottedException&) {
        refused = true;
    }
    check(refused, "re-running an allocated round without permission is refused");
    check(loadAllotments(conn, ROUND) == before, "refused re-run changed nothing");

    // With permission it runs -- which also proves the lock was released by
    // RunLock's destructor when the previous call threw.
    service.runAllocation(-1, ROUND, true);
    check(loadAllotments(conn, ROUND) == before,
          "explicit re-run gives identical allotments (deterministic)");
}

static void testConcurrentRunIsRefused(PostgresConnection& conn, const DbConfig& cfg) {
    std::cout << "testConcurrentRunIsRefused\n";
    const std::string lockRound = "TEST-ROUND-LOCK";

    // A second session holds the allocation lock, as if another run were active.
    PostgresConnection other(cfg);
    other.executeQuery("SELECT pg_advisory_lock(" +
                       std::to_string(AllocationService::RUN_LOCK_KEY) + ");");

    AllocationService service(conn);
    bool refused = false;
    try {
        service.runAllocation(-1, lockRound, true);
    } catch (const AllocationException::AlreadyAllottedException&) {
        refused = false; // wrong reason
    } catch (const AllocationException&) {
        refused = true;
    }
    check(refused, "a run is refused while another session holds the lock");
    check(loadAllotments(conn, lockRound).empty(), "the refused run wrote nothing");

    other.executeQuery("SELECT pg_advisory_unlock(" +
                       std::to_string(AllocationService::RUN_LOCK_KEY) + ");");

    AllocationSummary summary = service.runAllocation(-1, lockRound, true);
    check(summary.totalStudents > 0, "once the lock is free the run succeeds");
}

int main() {
    std::cout << "=== AllotMate end-to-end allocation tests ===\n";

    DbConfig cfg = DbConfig::fromEnv();
    try {
        PostgresConnection probe(cfg);
    } catch (const std::exception& e) {
        std::cout << "  [SKIP] no database reachable (" << e.what() << ")\n";
        return 0;
    }

    PostgresConnection conn(cfg);
    pqxx::result real = conn.executeQuery(
        "SELECT count(*) FROM ALLOTMENT WHERE round NOT LIKE 'TEST-ROUND%';");
    if (real[0][0].as<int>() > 0) {
        std::cout << "  [SKIP] database holds real allotments; this test only runs on seed data\n";
        return 0;
    }

    restoreSeedState(conn); // clear leftovers from an interrupted earlier run
    try {
        testRunProducesValidStableResult(conn);
        testWaitlistsFollowTheResult(conn);
        testOracleDetectsABlockingPair(conn);
        testRerunNeedsExplicitPermission(conn);
        testConcurrentRunIsRefused(conn, cfg);
    } catch (const std::exception& e) {
        std::cout << "  [FAIL] unexpected exception: " << e.what() << "\n";
        g_failures++;
    }
    restoreSeedState(conn);

    std::cout << "=============================================\n";
    if (g_failures == 0) {
        std::cout << "All allocation checks passed.\n";
        return 0;
    }
    std::cout << g_failures << " check(s) failed.\n";
    return 1;
}
