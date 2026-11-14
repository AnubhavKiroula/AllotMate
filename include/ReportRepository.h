#pragma once

#include <string>
#include <vector>
#include "PostgresConnection.h"

// One line of the "View allotment results" screen.
struct AllotmentRow {
    std::string rollNo;
    std::string studentName;
    std::string branch;
    double cgpa;
    int electiveId;
    std::string electiveName;
};

// A student shown in a report without an elective (unmatched list).
struct StudentRow {
    std::string rollNo;
    std::string studentName;
    std::string branch;
    double cgpa;
};

// One line of the "View waitlist" screen.
struct WaitlistRow {
    int position;
    std::string rollNo;
    std::string studentName;
    double cgpa;
};

// One elective a student is waiting for ("My Allotment Status" screen, FR-2).
struct WaitingForRow {
    std::string electiveName;
    int choiceRank;    // the student's own rank for this elective (1 = first choice)
    int position;      // place in that elective's waitlist (1 = next in line)
    int waitingTotal;  // how many students wait for that elective
};

// Read-only SQL queries behind the CLI report screens (owned by the CLI
// module). Keeping the SQL here means the CLI only formats rows and never
// builds queries itself.
class ReportRepository {
private:
    PostgresConnection& db;

public:
    explicit ReportRepository(PostgresConnection& connection);

    // Allotments for a round, grouped by elective, best CGPA first.
    // Pass electiveId = 0 for all electives, or an id to filter to one.
    std::vector<AllotmentRow> allotmentResults(const std::string& round, int electiveId = 0);

    // Students with no allotment in this round (SRS edge cases 1 and 5:
    // unmatched students must be reported, not silently dropped).
    std::vector<StudentRow> unmatchedStudents(const std::string& round);

    // An elective's waitlist, front of the line first.
    std::vector<WaitlistRow> waitlistFor(int electiveId);

    // Every elective one student is waiting for, their best-ranked choice first.
    std::vector<WaitingForRow> waitlistPositionsOf(const std::string& rollNo);
};
