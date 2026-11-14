#include "WaitlistRepository.h"
#include "AllocationException.h"

#include <sstream>

WaitlistRepository::WaitlistRepository(PostgresConnection& connection)
    : db(connection) {}

void WaitlistRepository::replaceWaitlist(pqxx::work& txn, int electiveId,
                                         const std::vector<int>& orderedStudentIds) {
    txn.exec("DELETE FROM WAITLIST WHERE elective_id = " + std::to_string(electiveId) + ";");

    if (!orderedStudentIds.empty()) {
        std::ostringstream sql;
        sql << "INSERT INTO WAITLIST (student_id, elective_id, position) VALUES ";
        for (size_t i = 0; i < orderedStudentIds.size(); i++) {
            if (i > 0) {
                sql << ", ";
            }
            sql << "(" << orderedStudentIds[i] << ", " << electiveId << ", " << (i + 1) << ")";
        }
        sql << ";";
        txn.exec(sql.str());
    }
}

void WaitlistRepository::replaceWaitlist(int electiveId, const std::vector<int>& orderedStudentIds) {
    try {
        // One transaction for delete + insert. If the insert fails, txn is
        // destroyed without commit() and PostgreSQL rolls the delete back too.
        pqxx::work txn(db.getRawConnection());
        replaceWaitlist(txn, electiveId, orderedStudentIds);
        txn.commit();
    } catch (const AllocationException&) {
        throw;
    } catch (const std::exception& e) {
        throw AllocationException::DatabaseException(
            "Failed to replace waitlist for elective " + std::to_string(electiveId) + ": " + e.what());
    }
}

std::vector<int> WaitlistRepository::loadWaitlist(int electiveId) {
    pqxx::result rows = db.executeQuery(
        "SELECT student_id FROM WAITLIST WHERE elective_id = " + std::to_string(electiveId)
        + " ORDER BY position ASC;");

    std::vector<int> studentIds;
    studentIds.reserve(rows.size());
    for (const auto& row : rows) {
        studentIds.push_back(row["student_id"].as<int>());
    }
    return studentIds;
}
