#include "ReportRepository.h"

#include <sstream>

ReportRepository::ReportRepository(PostgresConnection& connection)
    : db(connection) {}

std::vector<AllotmentRow> ReportRepository::allotmentResults(const std::string& round, int electiveId) {
    std::ostringstream sql;
    sql << "SELECT s.roll_no, s.name, s.branch, s.cgpa, e.elective_id, e.name AS elective_name "
        << "FROM ALLOTMENT a "
        << "JOIN STUDENT s ON s.student_id = a.student_id "
        << "JOIN ELECTIVE e ON e.elective_id = a.elective_id "
        << "WHERE a.round = " << db.quoteLiteral(round);
    if (electiveId > 0) {
        sql << " AND a.elective_id = " << electiveId;
    }
    sql << " ORDER BY e.elective_id ASC, s.cgpa DESC, s.roll_no ASC;";

    pqxx::result rows = db.executeQuery(sql.str());
    std::vector<AllotmentRow> report;
    report.reserve(rows.size());
    for (const auto& row : rows) {
        AllotmentRow line;
        line.rollNo = row["roll_no"].as<std::string>();
        line.studentName = row["name"].as<std::string>();
        line.branch = row["branch"].as<std::string>();
        line.cgpa = row["cgpa"].as<double>();
        line.electiveId = row["elective_id"].as<int>();
        line.electiveName = row["elective_name"].as<std::string>();
        report.push_back(line);
    }
    return report;
}

std::vector<StudentRow> ReportRepository::unmatchedStudents(const std::string& round) {
    std::ostringstream sql;
    sql << "SELECT s.roll_no, s.name, s.branch, s.cgpa FROM STUDENT s "
        << "WHERE NOT EXISTS (SELECT 1 FROM ALLOTMENT a WHERE a.student_id = s.student_id "
        << "AND a.round = " << db.quoteLiteral(round) << ") "
        << "ORDER BY s.cgpa DESC, s.roll_no ASC;";

    pqxx::result rows = db.executeQuery(sql.str());
    std::vector<StudentRow> report;
    report.reserve(rows.size());
    for (const auto& row : rows) {
        StudentRow line;
        line.rollNo = row["roll_no"].as<std::string>();
        line.studentName = row["name"].as<std::string>();
        line.branch = row["branch"].as<std::string>();
        line.cgpa = row["cgpa"].as<double>();
        report.push_back(line);
    }
    return report;
}

std::vector<WaitlistRow> ReportRepository::waitlistFor(int electiveId) {
    std::ostringstream sql;
    sql << "SELECT w.position, s.roll_no, s.name, s.cgpa FROM WAITLIST w "
        << "JOIN STUDENT s ON s.student_id = w.student_id "
        << "WHERE w.elective_id = " << electiveId
        << " ORDER BY w.position ASC;";

    pqxx::result rows = db.executeQuery(sql.str());
    std::vector<WaitlistRow> report;
    report.reserve(rows.size());
    for (const auto& row : rows) {
        WaitlistRow line;
        line.position = row["position"].as<int>();
        line.rollNo = row["roll_no"].as<std::string>();
        line.studentName = row["name"].as<std::string>();
        line.cgpa = row["cgpa"].as<double>();
        report.push_back(line);
    }
    return report;
}

std::vector<WaitingForRow> ReportRepository::waitlistPositionsOf(const std::string& rollNo) {
    // roll_no is typed by the user, so it is always passed through quoteLiteral.
    std::ostringstream sql;
    sql << "SELECT e.name, p.rank, w.position, "
        << "(SELECT count(*) FROM WAITLIST w2 WHERE w2.elective_id = w.elective_id) AS waiting "
        << "FROM WAITLIST w "
        << "JOIN STUDENT s ON s.student_id = w.student_id "
        << "JOIN ELECTIVE e ON e.elective_id = w.elective_id "
        << "JOIN PREFERENCE p ON p.student_id = w.student_id AND p.elective_id = w.elective_id "
        << "WHERE s.roll_no = " << db.quoteLiteral(rollNo)
        << " ORDER BY p.rank ASC;";

    pqxx::result rows = db.executeQuery(sql.str());
    std::vector<WaitingForRow> report;
    report.reserve(rows.size());
    for (const auto& row : rows) {
        WaitingForRow line;
        line.electiveName = row["name"].as<std::string>();
        line.choiceRank = row["rank"].as<int>();
        line.position = row["position"].as<int>();
        line.waitingTotal = row["waiting"].as<int>();
        report.push_back(line);
    }
    return report;
}
