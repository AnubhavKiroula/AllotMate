#pragma once

#include <vector>
#include "Repository.h"
#include "Student.h"
#include "StudentRecord.h"
#include "PostgresConnection.h"

// Concrete PostgreSQL-backed repository for Student entities.
//
// Fulfils the generic Repository<Student> interface (Dependency Inversion:
// callers depend on the interface, not on this class). It borrows an existing
// PostgresConnection by reference rather than opening its own, so the same
// connection is reused across calls (see ConnectionPool).
//
// Note on the domain model: Student (owned by the OOP module) only carries
// id, name, cgpa, preference list and status. roll_no and branch are
// persistence-only columns, so full-fidelity rows enter through bulkInsert()
// (fed by the CSV importer), while save() persists the columns a Student
// object actually holds.
class PostgresStudentRepo : public Repository<Student> {
private:
    PostgresConnection& db;

    // Builds a Student from one STUDENT row and loads its ranked preferences.
    Student mapRow(int id, const std::string& name, double cgpa, const std::string& status);

public:
    explicit PostgresStudentRepo(PostgresConnection& connection);

    // Persists a single student's own columns (name, cgpa, status): UPDATE if
    // the row exists, otherwise INSERT. Preferences/allotments are not touched.
    void save(Student entity) override;

    // Loads one student by primary key (throws DatabaseException if not found).
    Student findById(int id) override;

    // Loads every student ordered by id, with preferences attached in rank order.
    std::vector<Student> findAll() override;

    // Bulk-loads validated CSV records in chunked multi-row INSERTs. Duplicate
    // roll numbers that reach the database are ignored (ON CONFLICT DO NOTHING).
    // Returns the number of rows actually inserted.
    int bulkInsert(const std::vector<StudentRecord>& records);
};
