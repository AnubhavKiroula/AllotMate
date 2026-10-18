#include "PostgresElectiveRepo.h"
#include "AllocationException.h"

#include <sstream>

PostgresElectiveRepo::PostgresElectiveRepo(PostgresConnection& connection)
    : db(connection) {}

Elective PostgresElectiveRepo::mapRow(int id, const std::string& name,
                                      int capacity, int seatsFilled) {
    Elective elective(id, name, capacity);
    // The schema guarantees 0 <= seats_filled <= capacity, so replaying
    // allocateSeat() the right number of times can never overflow.
    for (int i = 0; i < seatsFilled; i++) {
        elective.allocateSeat();
    }
    return elective;
}

void PostgresElectiveRepo::save(Elective entity) {
    const int id = entity.getId();
    const std::string name = db.quoteLiteral(entity.getName());

    // UPDATE first. If the new capacity drops below seats_filled the DB CHECK
    // rejects it and executeCommand surfaces a DatabaseException (fail loud).
    std::ostringstream update;
    update << "UPDATE ELECTIVE SET name = " << name
           << ", capacity = " << entity.getCapacity()
           << ", seats_filled = " << entity.getSeatsFilled()
           << " WHERE elective_id = " << id << ";";

    if (db.executeCommand(update.str()) > 0) {
        return;
    }

    // Insert with explicit id. The domain model has no semester field, so the
    // required column gets a placeholder; real electives are seeded with one.
    std::ostringstream insert;
    insert << "INSERT INTO ELECTIVE (elective_id, name, capacity, seats_filled, semester) VALUES ("
           << id << ", " << name << ", "
           << entity.getCapacity() << ", " << entity.getSeatsFilled() << ", "
           << db.quoteLiteral("UNKNOWN") << ");";
    db.executeCommand(insert.str());
}

Elective PostgresElectiveRepo::findById(int id) {
    std::ostringstream sql;
    sql << "SELECT elective_id, name, capacity, seats_filled FROM ELECTIVE WHERE elective_id = "
        << id << ";";
    pqxx::result rows = db.executeQuery(sql.str());

    if (rows.empty()) {
        throw AllocationException::DatabaseException(
            "Elective with id " + std::to_string(id) + " not found.");
    }

    const auto& row = rows[0];
    return mapRow(row["elective_id"].as<int>(),
                  row["name"].as<std::string>(),
                  row["capacity"].as<int>(),
                  row["seats_filled"].as<int>());
}

std::vector<Elective> PostgresElectiveRepo::findAll() {
    pqxx::result rows = db.executeQuery(
        "SELECT elective_id, name, capacity, seats_filled FROM ELECTIVE ORDER BY elective_id ASC;");

    std::vector<Elective> electives;
    electives.reserve(rows.size());
    for (const auto& row : rows) {
        electives.push_back(mapRow(row["elective_id"].as<int>(),
                                   row["name"].as<std::string>(),
                                   row["capacity"].as<int>(),
                                   row["seats_filled"].as<int>()));
    }
    return electives;
}
