// ============================================================
// test_csv_importer.cpp -- pure-logic unit tests (no database)
//
// Covers the two Phase 2 properties that can be checked without a
// live PostgreSQL server:
//   1. CSV import: validation, duplicate roll-number rejection, sorting.
//   2. Capacity:   an Elective never admits more students than its capacity.
//
// A tiny check() macro is used instead of a heavyweight test framework so
// the file stays readable and compiles anywhere the core sources do.
// ============================================================

#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#include "CsvImporter.h"
#include "Elective.h"
#include "AllocationException.h"

static int g_failures = 0;

#define check(cond, msg)                                           \
    do {                                                           \
        if (!(cond)) {                                             \
            std::cout << "  [FAIL] " << (msg) << "\n";             \
            g_failures++;                                          \
        } else {                                                   \
            std::cout << "  [ok]   " << (msg) << "\n";             \
        }                                                          \
    } while (0)

static void testValidRowsAreAcceptedAndSorted() {
    std::cout << "testValidRowsAreAcceptedAndSorted\n";
    std::stringstream csv;
    csv << "roll_no,name,cgpa,branch\n";          // header, should be skipped
    csv << "2024CS003,Charlie,7.80,CSE\n";
    csv << "2024CS001,Alice,9.10,IT\n";
    csv << "2024CS002,Bob,8.40,ECE\n";

    ImportResult r = CsvImporter::parseStream(csv);

    check(r.accepted.size() == 3, "three valid rows accepted");
    check(r.errors.empty(), "no errors on clean input");
    check(r.totalRows == 3, "header excluded from data-row count");
    // Sorted by roll number -> Alice, Bob, Charlie.
    check(r.accepted[0].rollNo == "2024CS001", "rows sorted by roll number (first)");
    check(r.accepted[2].rollNo == "2024CS003", "rows sorted by roll number (last)");
    check(r.accepted[0].cgpa > 9.0 && r.accepted[0].cgpa < 9.2, "cgpa parsed correctly");
}

static void testDuplicateRollNumbersAreRejected() {
    std::cout << "testDuplicateRollNumbersAreRejected\n";
    std::stringstream csv;
    csv << "2024CS001,Alice,9.10,IT\n";
    csv << "2024CS001,Alice Clone,9.10,IT\n";  // same roll number -> dropped
    csv << "2024CS002,Bob,8.40,ECE\n";

    ImportResult r = CsvImporter::parseStream(csv);

    check(r.accepted.size() == 2, "duplicate roll number dropped");
    check(r.duplicateCount == 1, "duplicate counted exactly once");
}

static void testMalformedRowsAreReportedNotCrashing() {
    std::cout << "testMalformedRowsAreReportedNotCrashing\n";
    std::stringstream csv;
    csv << "2024CS001,Alice,9.10,IT\n";        // valid
    csv << "2024CS002,Bob,8.40\n";             // missing branch -> error
    csv << "2024CS003,,7.80,CSE\n";            // empty name -> error
    csv << "2024CS004,Dan,notanumber,CSE\n";   // bad cgpa -> error
    csv << "2024CS005,Eve,11.50,CSE\n";        // cgpa out of range -> error

    ImportResult r = CsvImporter::parseStream(csv);

    check(r.accepted.size() == 1, "only the single valid row is accepted");
    check(r.errors.size() == 4, "all four bad rows are reported");
    // Line numbers must point back at the real offending lines.
    check(r.errors[0].lineNumber == 2, "error keeps original line number");
}

// Builds a record with only the roll number and name set (enough for sorting).
static StudentRecord rec(const std::string& roll, const std::string& name) {
    StudentRecord r;
    r.rollNo = roll;
    r.name = name;
    return r;
}

static bool isSortedByRoll(const std::vector<StudentRecord>& rows) {
    for (size_t i = 1; i < rows.size(); i++) {
        if (rows[i].rollNo < rows[i - 1].rollNo) {
            return false;
        }
    }
    return true;
}

static void testMergeSortEdgeCases() {
    std::cout << "testMergeSortEdgeCases\n";

    std::vector<StudentRecord> empty;
    CsvImporter::sortByRollNo(empty);
    check(empty.empty(), "empty input stays empty");

    std::vector<StudentRecord> one = { rec("R1", "Only") };
    CsvImporter::sortByRollNo(one);
    check(one.size() == 1 && one[0].rollNo == "R1", "single row unchanged");

    std::vector<StudentRecord> reversed = {
        rec("R5", "e"), rec("R4", "d"), rec("R3", "c"), rec("R2", "b"), rec("R1", "a")
    };
    CsvImporter::sortByRollNo(reversed);
    check(isSortedByRoll(reversed) && reversed[0].rollNo == "R1",
          "reverse-ordered input comes out ascending");

    std::vector<StudentRecord> sorted = { rec("R1", "a"), rec("R2", "b"), rec("R3", "c") };
    CsvImporter::sortByRollNo(sorted);
    check(isSortedByRoll(sorted) && sorted.size() == 3, "already-sorted input stays sorted");
}

static void testSortIsStableSoFirstDuplicateWins() {
    std::cout << "testSortIsStableSoFirstDuplicateWins\n";
    std::stringstream csv;
    csv << "2024CS009,Zed,7.00,CSE\n";
    csv << "2024CS005,First Copy,8.00,CSE\n";  // first occurrence -> must be kept
    csv << "2024CS001,Ann,9.00,IT\n";
    csv << "2024CS005,Second Copy,8.00,CSE\n"; // later duplicate -> dropped

    ImportResult r = CsvImporter::parseStream(csv);

    bool keptFirst = false;
    for (const StudentRecord& s : r.accepted) {
        if (s.rollNo == "2024CS005") {
            keptFirst = (s.name == "First Copy");
        }
    }
    check(r.duplicateCount == 1, "one duplicate dropped");
    check(keptFirst, "stable sort keeps the FIRST occurrence from the file");
}

static void testElectiveCapacityIsEnforced() {
    std::cout << "testElectiveCapacityIsEnforced\n";
    Elective e(1, "Advanced Machine Learning", 2);

    check(e.hasSeat(), "new elective reports a free seat");
    e.allocateSeat();
    e.allocateSeat();                 // now full (capacity == 2)
    check(!e.hasSeat(), "full elective reports no free seat");
    check(e.getSeatsFilled() == e.getCapacity(), "seats filled never exceeds capacity");

    bool threw = false;
    try {
        e.allocateSeat();             // one past capacity -> must throw, not overflow
    } catch (const AllocationException::SeatOverflowException&) {
        threw = true;
    }
    check(threw, "allocating past capacity throws SeatOverflowException");
    check(e.getSeatsFilled() == 2, "seats filled unchanged after a rejected allocation");
}

int main() {
    std::cout << "=== AllotMate CSV + capacity unit tests ===\n";
    testValidRowsAreAcceptedAndSorted();
    testDuplicateRollNumbersAreRejected();
    testMalformedRowsAreReportedNotCrashing();
    testMergeSortEdgeCases();
    testSortIsStableSoFirstDuplicateWins();
    testElectiveCapacityIsEnforced();

    std::cout << "===========================================\n";
    if (g_failures == 0) {
        std::cout << "All checks passed.\n";
        return 0;
    }
    std::cout << g_failures << " check(s) failed.\n";
    return 1;
}
