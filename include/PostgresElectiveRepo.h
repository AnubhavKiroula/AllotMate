#pragma once

#include <vector>
#include "Repository.h"
#include "Elective.h"
#include "PostgresConnection.h"

// Concrete PostgreSQL-backed repository for Elective entities.
// Mirrors PostgresStudentRepo: it borrows a PostgresConnection by reference and
// implements the generic Repository<Elective> interface.
class PostgresElectiveRepo : public Repository<Elective> {
private:
    PostgresConnection& db;

    // Builds an Elective and replays allocateSeat() so its filled-seat count
    // matches the stored seats_filled (the model exposes no direct setter).
    Elective mapRow(int id, const std::string& name, int capacity, int seatsFilled);

public:
    explicit PostgresElectiveRepo(PostgresConnection& connection);

    // UPDATEs name/capacity/seats_filled if the elective exists, else INSERTs it.
    void save(Elective entity) override;

    Elective findById(int id) override;
    std::vector<Elective> findAll() override;
};
