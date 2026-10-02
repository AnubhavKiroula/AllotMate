#pragma once

#include <string>

// Plain data holder for one row of the student import CSV.
// Kept deliberately free of any database/libpqxx types so the parsing
// and validation logic can be unit-tested without a live PostgreSQL server.
struct StudentRecord {
    std::string rollNo;
    std::string name;
    double cgpa = 0.0;
    std::string branch;
};
