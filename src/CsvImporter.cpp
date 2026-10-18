#include "CsvImporter.h"

#include <fstream>
#include <sstream>
#include <stdexcept>
#include <utility>

std::string CsvImporter::trim(const std::string& s) {
    size_t first = s.find_first_not_of(" \t\r\n");
    if (first == std::string::npos) {
        return "";
    }
    size_t last = s.find_last_not_of(" \t\r\n");
    return s.substr(first, last - first + 1);
}

bool CsvImporter::parseLine(const std::string& line, StudentRecord& out, std::string& reason) {
    // Split the line into comma-separated fields.
    std::vector<std::string> fields;
    std::stringstream ss(line);
    std::string field;
    while (std::getline(ss, field, ',')) {
        fields.push_back(trim(field));
    }

    if (fields.size() != 4) {
        reason = "expected 4 fields (roll_no,name,cgpa,branch) but found "
                 + std::to_string(fields.size());
        return false;
    }

    const std::string& rollNo = fields[0];
    const std::string& name = fields[1];
    const std::string& cgpaText = fields[2];
    const std::string& branch = fields[3];

    if (rollNo.empty()) { reason = "roll number is empty"; return false; }
    if (name.empty())   { reason = "name is empty"; return false; }
    if (branch.empty()) { reason = "branch is empty"; return false; }

    // Parse CGPA and make sure the whole field was a number (std::stod would
    // happily accept "8.5abc"), then bound it to the 0-10 range the schema allows.
    double cgpa = 0.0;
    try {
        size_t consumed = 0;
        cgpa = std::stod(cgpaText, &consumed);
        if (consumed != cgpaText.size()) {
            reason = "cgpa '" + cgpaText + "' is not a plain number";
            return false;
        }
    } catch (const std::exception&) {
        reason = "cgpa '" + cgpaText + "' is not a number";
        return false;
    }

    if (cgpa < 0.0 || cgpa > 10.0) {
        reason = "cgpa " + cgpaText + " is outside the valid 0-10 range";
        return false;
    }

    out.rollNo = rollNo;
    out.name = name;
    out.cgpa = cgpa;
    out.branch = branch;
    return true;
}

ImportResult CsvImporter::parseStream(std::istream& in) {
    ImportResult result;

    std::string line;
    int lineNumber = 0;
    while (std::getline(in, line)) {
        lineNumber++;

        std::string trimmed = trim(line);
        if (trimmed.empty()) {
            continue; // ignore blank lines without counting them as data
        }

        // Skip a header row if the file ships with one.
        if (lineNumber == 1 && trimmed.rfind("roll_no", 0) == 0) {
            continue;
        }

        result.totalRows++;

        StudentRecord record;
        std::string reason;
        if (parseLine(trimmed, record, reason)) {
            result.accepted.push_back(record);
        } else {
            result.errors.push_back(RowError{ lineNumber, line, reason });
        }
    }

    // Sort valid rows by roll number so duplicates sit next to each other.
    // Sorting here is the DS concept for this module: an O(n log n) sort turns
    // duplicate detection into a single linear scan over adjacent rows.
    sortByRollNo(result.accepted);

    std::vector<StudentRecord> unique;
    unique.reserve(result.accepted.size());
    for (size_t i = 0; i < result.accepted.size(); i++) {
        if (i > 0 && result.accepted[i].rollNo == result.accepted[i - 1].rollNo) {
            result.duplicateCount++; // keep the first occurrence, drop the rest
            continue;
        }
        unique.push_back(result.accepted[i]);
    }
    result.accepted = std::move(unique);

    return result;
}

void CsvImporter::sortByRollNo(std::vector<StudentRecord>& rows) {
    std::vector<StudentRecord> temp(rows.size());
    mergeSort(rows, temp, 0, rows.size());
}

void CsvImporter::mergeSort(std::vector<StudentRecord>& rows, std::vector<StudentRecord>& temp,
                            size_t left, size_t right) {
    if (right - left < 2) {
        return; // zero or one row is already sorted
    }
    size_t mid = left + (right - left) / 2;
    mergeSort(rows, temp, left, mid);
    mergeSort(rows, temp, mid, right);
    merge(rows, temp, left, mid, right);
}

void CsvImporter::merge(std::vector<StudentRecord>& rows, std::vector<StudentRecord>& temp,
                        size_t left, size_t mid, size_t right) {
    size_t i = left; // walks the left half
    size_t j = mid;  // walks the right half
    size_t k = left; // next free slot in temp

    while (i < mid && j < right) {
        // Strict '<' means a tie takes the LEFT row first -- this is what makes
        // the sort stable.
        if (rows[j].rollNo < rows[i].rollNo) {
            temp[k++] = rows[j++];
        } else {
            temp[k++] = rows[i++];
        }
    }
    while (i < mid) {
        temp[k++] = rows[i++];
    }
    while (j < right) {
        temp[k++] = rows[j++];
    }

    for (k = left; k < right; k++) {
        rows[k] = temp[k];
    }
}

ImportResult CsvImporter::parseFile(const std::string& path) {
    std::ifstream file(path);
    if (!file.is_open()) {
        throw std::runtime_error("CsvImporter: could not open file '" + path + "'");
    }
    return parseStream(file);
}
