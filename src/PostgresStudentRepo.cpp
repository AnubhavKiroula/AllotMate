#include "PostgresStudentRepo.h"
#include "AllocationException.h"

#include <sstream>
#include <iomanip>
#include <algorithm>
#include <utility>

namespace {
    // DB status text <-> Status enum. The seed uses 'UNASSIGNED'; an allotted
    // student is stored as 'ALLOTTED'.
    std::string statusToText(Status s) {
        return (s == Status::ALLOTED) ? "ALLOTTED" : "UNASSIGNED";
    }

    Status statusFromText(const std::string& text) {
        // Treat anything starting with "ALLOT"/"ALLOC" as allotted; else unmatched.
        if (text.rfind("ALLOT", 0) == 0 || text.rfind("ALLOC", 0) == 0) {
            return Status::ALLOTED;
        }
        return Status::UNMATCHED;
    }

    // Format a CGPA to 2 decimals so it fits the NUMERIC(4,2) column cleanly.
    std::string cgpaToText(double cgpa) {
        std::ostringstream oss;
        oss << std::fixed << std::setprecision(2) << cgpa;
        return oss.str();
    }
}

PostgresStudentRepo::PostgresStudentRepo(PostgresConnection& connection)
    : db(connection) {}

Student PostgresStudentRepo::mapRow(int id, const std::string& name, double cgpa,
                                    const std::string& status) {
    Student student(id, name, static_cast<float>(cgpa));
    student.setStatus(statusFromText(status));

    // Attach this student's preferences in rank order.
    std::ostringstream sql;
    sql << "SELECT elective_id FROM PREFERENCE WHERE student_id = " << id
        << " ORDER BY rank ASC;";
    pqxx::result prefs = db.executeQuery(sql.str());
    for (const auto& row : prefs) {
        student.addPreference(row["elective_id"].as<int>());
    }
    return student;
}

void PostgresStudentRepo::save(Student entity) {
    const int id = entity.getId();
    const std::string name = db.quoteLiteral(entity.getName());
    const std::string cgpa = cgpaToText(entity.getCgpa());
    const std::string status = db.quoteLiteral(statusToText(entity.getStatus()));

    // Try an UPDATE first; the common case is persisting an existing student's
    // status/cgpa after a matching run.
    std::ostringstream update;
    update << "UPDATE STUDENT SET name = " << name
           << ", cgpa = " << cgpa
           << ", status = " << status
           << " WHERE student_id = " << id << ";";

    if (db.executeCommand(update.str()) > 0) {
        return;
    }

    // No row existed: insert with the explicit id. The domain model has no
    // roll_no/branch, so we fill required columns with deterministic placeholders
    // (real rows come through bulkInsert()).
    std::ostringstream insert;
    insert << "INSERT INTO STUDENT (student_id, name, roll_no, cgpa, branch, status) VALUES ("
           << id << ", " << name << ", "
           << db.quoteLiteral("GEN" + std::to_string(id)) << ", "
           << cgpa << ", " << db.quoteLiteral("UNKNOWN") << ", " << status << ");";
    db.executeCommand(insert.str());
}

Student PostgresStudentRepo::findById(int id) {
    std::ostringstream sql;
    sql << "SELECT student_id, name, cgpa, status FROM STUDENT WHERE student_id = "
        << id << ";";
    pqxx::result rows = db.executeQuery(sql.str());

    if (rows.empty()) {
        throw AllocationException::DatabaseException(
            "Student with id " + std::to_string(id) + " not found.");
    }

    const auto& row = rows[0];
    return mapRow(row["student_id"].as<int>(),
                  row["name"].as<std::string>(),
                  row["cgpa"].as<double>(),
                  row["status"].as<std::string>());
}

std::vector<Student> PostgresStudentRepo::findAll() {
    pqxx::result rows = db.executeQuery(
        "SELECT student_id, name, cgpa, status FROM STUDENT ORDER BY student_id ASC;");

    std::vector<Student> students;
    students.reserve(rows.size());

    for (const auto& row : rows) {
        Student student(row["student_id"].as<int>(), row["name"].as<std::string>(),
                        static_cast<float>(row["cgpa"].as<double>()));
        student.setStatus(statusFromText(row["status"].as<std::string>()));
        students.push_back(student);
    }

    // One query for all preferences (instead of one per student) keeps findAll()
    // at two round trips even at 1,500-student scale.
    pqxx::result prefs = db.executeQuery(
        "SELECT student_id, elective_id FROM PREFERENCE ORDER BY student_id, rank ASC;");

    // Merge-join: students and preferences are both sorted by student_id, so a
    // single index `s` walks the students while we walk the preferences -- the
    // same idea as the merge step of merge sort. O(students + preferences) and
    // no lookup table needed.
    size_t s = 0;
    for (const auto& row : prefs) {
        int studentId = row["student_id"].as<int>();
        while (s < students.size() && students[s].getId() < studentId) {
            s++;
        }
        if (s < students.size() && students[s].getId() == studentId) {
            students[s].addPreference(row["elective_id"].as<int>());
        }
    }

    return students;
}

int PostgresStudentRepo::bulkInsert(const std::vector<StudentRecord>& records) {
    if (records.empty()) {
        return 0;
    }

    const size_t chunkSize = 500; // keep each INSERT statement a reasonable size
    int inserted = 0;

    for (size_t start = 0; start < records.size(); start += chunkSize) {
        size_t end = std::min(start + chunkSize, records.size());

        std::ostringstream sql;
        sql << "INSERT INTO STUDENT (name, roll_no, cgpa, branch, status) VALUES ";
        for (size_t i = start; i < end; i++) {
            const StudentRecord& r = records[i];
            if (i > start) {
                sql << ", ";
            }
            sql << "(" << db.quoteLiteral(r.name) << ", "
                << db.quoteLiteral(r.rollNo) << ", "
                << cgpaToText(r.cgpa) << ", "
                << db.quoteLiteral(r.branch) << ", "
                << db.quoteLiteral("UNASSIGNED") << ")";
        }
        // A duplicate roll number that slipped past the importer (e.g. already
        // seeded) is skipped here rather than aborting the whole batch.
        sql << " ON CONFLICT (roll_no) DO NOTHING;";

        inserted += db.executeCommand(sql.str());
    }

    return inserted;
}
