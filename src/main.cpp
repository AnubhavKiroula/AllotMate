#include <iostream>
#include <iomanip>
#include "PostgresConnection.h"
#include "AllocationException.h"
#include "Student.h"
#include "Coordinator.h"
#include "Elective.h"
#include "MatchingEngine.h"

int main() {
    std::cout << "========================================" << std::endl;
    std::cout << "   AllotMate - DB Layer Connection Test  " << std::endl;
    std::cout << "========================================" << std::endl;

    try {
        std::cout << "[1/4] Initializing PostgreSQL connection from .env..." << std::endl;
        PostgresConnection db;
        std::cout << " -> Successfully connected to PostgreSQL server." << std::endl;

        std::cout << "[2/4] Querying database version and table counts..." << std::endl;
        pqxx::result verResult = db.executeQuery("SELECT version();");
        if (!verResult.empty()) {
            std::cout << " -> DB Server Version: " << verResult[0][0].c_str() << std::endl;
        }

        pqxx::result studentCount = db.executeQuery("SELECT COUNT(*) FROM STUDENT;");
        pqxx::result electiveCount = db.executeQuery("SELECT COUNT(*) FROM ELECTIVE;");
        pqxx::result prefCount = db.executeQuery("SELECT COUNT(*) FROM PREFERENCE;");

        std::cout << " -> Current Records in DB:" << std::endl;
        std::cout << "    - Students:    " << studentCount[0][0].c_str() << std::endl;
        std::cout << "    - Electives:   " << electiveCount[0][0].c_str() << std::endl;
        std::cout << "    - Preferences: " << prefCount[0][0].c_str() << std::endl;

        std::cout << "[3/4] Performing Phase 1 Student Insert & Read Verification Cycle..." << std::endl;
        // Insert a test student with unique roll number
        std::string testRoll = "2024TEST999";
        db.executeCommand("DELETE FROM STUDENT WHERE roll_no = '" + testRoll + "';");
        
        int rowsInserted = db.executeCommand(
            "INSERT INTO STUDENT (name, roll_no, cgpa, branch, status) "
            "VALUES ('Test Student', '" + testRoll + "', 9.75, 'Computer Science & Engineering', 'UNASSIGNED');"
        );
        std::cout << " -> Inserted " << rowsInserted << " test student row." << std::endl;

        pqxx::result readResult = db.executeQuery(
            "SELECT student_id, name, roll_no, cgpa, branch, status FROM STUDENT WHERE roll_no = '" + testRoll + "';"
        );

        if (!readResult.empty()) {
            std::cout << " -> Read back test student successfully:" << std::endl;
            std::cout << "    ID:     " << readResult[0]["student_id"].c_str() << std::endl;
            std::cout << "    Name:   " << readResult[0]["name"].c_str() << std::endl;
            std::cout << "    Roll:   " << readResult[0]["roll_no"].c_str() << std::endl;
            std::cout << "    CGPA:   " << readResult[0]["cgpa"].c_str() << std::endl;
            std::cout << "    Branch: " << readResult[0]["branch"].c_str() << std::endl;
            std::cout << "    Status: " << readResult[0]["status"].c_str() << std::endl;
        }

        // Clean up test student
        db.executeCommand("DELETE FROM STUDENT WHERE roll_no = '" + testRoll + "';");
        std::cout << " -> Cleaned up test record." << std::endl;

        std::cout << "[4/4] Phase 1 Database Connection and CRUD verification completed successfully." << std::endl;

        std::cout << "\n[5/5] Verifying Phase 2 domain models..." << std::endl;

        Student student(1, "Test Student" , 9.5f);
        Coordinator coordinator(1,"Test Coordinator" , "CSE");
        Elective elective(1,"Data Structures", 50);

        Person* studentProfile = &student;
        Person* coordinatorProfile = &coordinator;

        studentProfile->displayProfile();
        coordinatorProfile->displayProfile();

        if(elective.hasSeat()){
            std::cout << "-> Elective seat availability check: PASS" << std::endl;
        }
        else{
            std::cout << "-> Elective seat availability check: FAIL" << std::endl;
        }

        MatchingEngine engine;
        engine.runGaleShapley();
        std::cout << "-> MatchingEngine stub initialization: PASS" << std::endl;

        std::cout << "-> Phase 2 domain model verification completed." << std::endl;

        return 0;

    } catch (const AllocationException::DatabaseException& e) {
        std::cerr << "\n[ERROR] Database Exception caught: " << e.what() << std::endl;
        std::cerr << "Hint: Ensure PostgreSQL Docker container is running ('docker compose up -d') and credentials in .env are correct." << std::endl;
        return 1;
    } catch (const AllocationException& e) {
        std::cerr << "\n[ERROR] Domain Allocation Exception caught: " << e.what() << std::endl;
        return 1;
    } catch (const std::exception& e) {
        std::cerr << "\n[ERROR] Standard Exception caught: " << e.what() << std::endl;
        return 1;
    }
}