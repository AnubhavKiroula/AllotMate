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
    testElectiveCapacityIsEnforced();

    std::cout << "===========================================\n";
    if (g_failures == 0) {
        std::cout << "All checks passed.\n";
        return 0;
    }
    std::cout << g_failures << " check(s) failed.\n";
    return 1;
}
