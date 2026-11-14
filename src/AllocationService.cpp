#include "AllocationService.h"
#include "AllocationException.h"
#include "PostgresStudentRepo.h"
#include "PostgresElectiveRepo.h"
#include "RunLogRepository.h"

#include <sstream>
#include <vector>

// ---- RunLock ----

RunLock::RunLock(PostgresConnection& connection) : db(connection) {
    pqxx::result res = db.executeQuery(
        "SELECT pg_try_advisory_lock(" + std::to_string(AllocationService::RUN_LOCK_KEY) + ");");
    if (!res[0][0].as<bool>()) {
        throw AllocationException(
            "Another allocation run is already in progress. Wait for it to finish.");
    }
}

RunLock::~RunLock() {
    try {
        db.executeQuery(
            "SELECT pg_advisory_unlock(" + std::to_string(AllocationService::RUN_LOCK_KEY) + ");");
    } catch (...) {
        // Never throw from a destructor. If the connection is gone, PostgreSQL
        // releases session locks automatically when the session ends.
    }
}

// ---- AllocationService ----

AllocationService::AllocationService(PostgresConnection& connection)
    : db(connection) {}

bool AllocationService::roundAlreadyAllocated(const std::string& round) {
    pqxx::result res = db.executeQuery(
        "SELECT count(*) FROM ALLOTMENT WHERE round = " + db.quoteLiteral(round) + ";");
    return res[0][0].as<int>() > 0;
}

int AllocationService::saveResults(const MatchingEngine& engine, const std::string& round) {
    const std::string roundText = db.quoteLiteral(round);

    // Build the allotment rows from the engine's answer for each student.
    std::ostringstream insert;
    insert << "INSERT INTO ALLOTMENT (student_id, elective_id, round) VALUES ";
    int matched = 0;
    for (const Student& student : engine.getStudents()) {
        int electiveId = engine.getAssignedElectiveId(student.getId());
        if (electiveId == -1) {
            continue; // unmatched: no allotment row, reported via the summary
        }
        if (matched > 0) {
            insert << ", ";
        }
        insert << "(" << student.getId() << ", " << electiveId << ", " << roundText << ")";
        matched++;
    }
    insert << ";";

    try {
        // Everything below commits together or not at all.
        pqxx::work txn(db.getRawConnection());

        txn.exec("DELETE FROM ALLOTMENT WHERE round = " + roundText + ";");
        if (matched > 0) {
            txn.exec(insert.str());
        }

        txn.exec("UPDATE STUDENT SET status = 'UNASSIGNED';");
        txn.exec("UPDATE STUDENT SET status = 'ALLOTTED' WHERE student_id IN "
                 "(SELECT student_id FROM ALLOTMENT WHERE round = " + roundText + ");");

        // seats_filled is recounted from the saved rows, so it can never drift
        // from ALLOTMENT. The CHECK (seats_filled <= capacity) is a last guard.
        txn.exec("UPDATE ELECTIVE e SET seats_filled = "
                 "(SELECT count(*) FROM ALLOTMENT a WHERE a.elective_id = e.elective_id "
                 "AND a.round = " + roundText + ");");

        txn.commit();
    } catch (const std::exception& e) {
        throw AllocationException::DatabaseException(
            std::string("Saving allocation results failed (nothing was saved): ") + e.what());
    }

    return matched;
}

AllocationSummary AllocationService::runAllocation(int coordinatorId, const std::string& round,
                                                   bool allowRerun) {
    RunLock lock(db); // released automatically when this function returns or throws

    if (!allowRerun && roundAlreadyAllocated(round)) {
        throw AllocationException::AlreadyAllottedException(
            "Round '" + round + "' is already allocated. Re-running it replaces the "
            "existing results, so it must be confirmed explicitly.");
    }

    PostgresStudentRepo studentRepo(db);
    PostgresElectiveRepo electiveRepo(db);
    std::vector<Student> students = studentRepo.findAll();
    std::vector<Elective> electives = electiveRepo.findAll();

    MatchingEngine engine;
    engine.loadData(students, electives);
    engine.runGaleShapley();

    AllocationSummary summary;
    summary.fairness = engine.generateFairnessReport();
    summary.totalStudents = static_cast<int>(students.size());
    summary.matched = saveResults(engine, round);
    summary.unmatched = summary.totalStudents - summary.matched;

    const int blockingPairs = summary.fairness.blockingPairs;
    std::ostringstream outcome;
    outcome << round << ": " << summary.matched << " of " << summary.totalStudents
            << " students matched, " << summary.unmatched << " unmatched. "
            << "Blocking pairs found: " << blockingPairs << ".";

    RunLogRepository runLog(db);
    summary.runId = runLog.logRun(coordinatorId, blockingPairs, outcome.str());
    return summary;
}
