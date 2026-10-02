#pragma once

#include <string>
#include <vector>
#include <istream>
#include "StudentRecord.h"

// One row that failed validation, kept so the coordinator can see exactly
// which line was rejected and why instead of a silent skip.
struct RowError {
    int lineNumber;      // 1-based line number in the source file
    std::string rawLine; // the offending line, untouched
    std::string reason;  // human-readable explanation
};

// Result of parsing a student CSV file.
struct ImportResult {
    std::vector<StudentRecord> accepted; // valid rows, de-duplicated and sorted by roll number
    std::vector<RowError> errors;        // malformed / out-of-range rows
    int duplicateCount = 0;              // rows dropped because their roll number repeated
    int totalRows = 0;                   // data rows read (header and blank lines excluded)
};

// CsvImporter turns a CSV file of students into validated records.
//
// It does NOT talk to the database on purpose: parsing + validation is pure
// logic, so PostgresStudentRepo::bulkInsert handles persistence separately.
// Expected columns (comma separated): roll_no, name, cgpa, branch.
// A leading header row (first field == "roll_no") is detected and skipped.
class CsvImporter {
public:
    // Reads and parses the file at `path`. Throws std::runtime_error if the
    // file cannot be opened (fail loud rather than returning an empty result).
    static ImportResult parseFile(const std::string& path);

    // Parses already-opened CSV content. Useful for tests (feed a stringstream).
    static ImportResult parseStream(std::istream& in);

private:
    // Splits one line into a record. Returns false and fills `reason` on any
    // validation failure (wrong field count, empty field, bad/out-of-range CGPA).
    static bool parseLine(const std::string& line, StudentRecord& out, std::string& reason);

    static std::string trim(const std::string& s);
};
