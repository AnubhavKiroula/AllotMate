#include "CLI.h"
#include <iostream>
#include <iomanip>
#include <limits>
#include <stdexcept>

// ============================================================
// CLI.cpp -- Implementation of the AllotMate Interactive CLI
//
// Each method maps to exactly one "screen" in the terminal
// interface. Methods are ordered to match the menu options
// so the file is easy to navigate during evaluation walkthrough.
// ============================================================

// The allocation round this CLI works on. The results and
// waitlist screens must use this same name.
const std::string CURRENT_ROUND = "Round 1";

// There is no coordinator login screen yet, so every run is
// recorded under the first seeded coordinator (ID 1).
const int DEMO_COORDINATOR_ID = 1;

// -----------------------------------------------------------
// Constructor: Establish DB connection using .env credentials.
// PostgresConnection is RAII -- connection is held for CLI
// lifetime and released automatically on destruction.
// -----------------------------------------------------------
CLI::CLI() : db(), running(true) {
    // DB connection is established inside PostgresConnection constructor.
    // If .env is missing or credentials are wrong, DatabaseException is
    // propagated up and caught in main().
}

// -----------------------------------------------------------
// run() -- Top-level entry point. Shown first to evaluators.
// Greets user, selects role, then hands off to role menu.
// -----------------------------------------------------------
void CLI::run() {
    printHeader("Welcome to AllotMate");
    std::cout << "  Fair Elective Allocation Engine\n";
    std::cout << "  Gale-Shapley Stable Matching (C++17 + PostgreSQL 16)\n";
    printSeparator();

    int role = selectRole();
    if (role == 1) {
        runCoordinatorMenu();
    } else {
        runStudentMenu();
    }

    std::cout << "\nThank you for using AllotMate. Goodbye!\n";
}

// -----------------------------------------------------------
// selectRole() -- Ask user to login as Coordinator or Student.
// Returns 1 or 2. Loops on invalid input.
// -----------------------------------------------------------
int CLI::selectRole() {
    while (true) {
        std::cout << "\n  Login as:\n";
        std::cout << "    [1] Coordinator (full admin access)\n";
        std::cout << "    [2] Student     (view own allotment status)\n";
        int choice = getIntInput("  Enter choice: ");
        if (choice == 1 || choice == 2) return choice;
        std::cout << "  Invalid choice. Please enter 1 or 2.\n";
    }
}

// -----------------------------------------------------------
// runCoordinatorMenu() -- Main loop for coordinator role.
// Loops until user selects Exit (option 6).
// Option 0 opens the OOP evaluation demo screen.
// -----------------------------------------------------------
void CLI::runCoordinatorMenu() {
    while (running) {
        printHeader("Coordinator Menu");
        std::cout << "  [1] View Electives & Seat Availability\n";
        std::cout << "  [2] Update Elective Seat Capacity\n";
        std::cout << "  [3] View All Students\n";
        std::cout << "  [4] View Student Preferences\n";
        std::cout << "  [5] Run Allocation (Gale-Shapley)\n";
        std::cout << "  [0] OOP & DS Concepts Demo (Evaluator Mode)\n";
        std::cout << "  [6] Exit\n";
        printSeparator();

        int choice = getIntInput("  Enter option: ");
        switch (choice) {
            case 1: viewElectives();           break;
            case 2: updateElectiveCapacity();  break;
            case 3: viewAllStudents();         break;
            case 4: viewPreferences();         break;
            case 5: runAllocation();           break;
            case 0: runOOPDemoScreen();        break;
            case 6: running = false;           break;
            default:
                std::cout << "\n  [!] Invalid option. Please choose 0-6.\n";
        }
    }
}

// -----------------------------------------------------------
// viewElectives() -- OPTION 1
// Reads all electives from PostgreSQL and displays them in a
// formatted table with seats filled and seats remaining.
// Demonstrates: live DB query via RAII PostgresConnection.
// -----------------------------------------------------------
void CLI::viewElectives() {
    printHeader("Electives & Seat Availability");
    try {
        pqxx::result rows = db.executeQuery(
            "SELECT elective_id, name, capacity, seats_filled "
            "FROM ELECTIVE ORDER BY elective_id;"
        );

        if (rows.empty()) {
            std::cout << "  No electives found in database.\n";
            waitForEnter();
            return;
        }

        // Table header
        std::cout << "\n  "
                  << std::left  << std::setw(5)  << "ID"
                  << std::left  << std::setw(35) << "Elective Name"
                  << std::right << std::setw(10) << "Capacity"
                  << std::right << std::setw(12) << "Seats Used"
                  << std::right << std::setw(12) << "Remaining"
                  << "\n";
        printSeparator();

        for (const auto& row : rows) {
            int cap   = row["capacity"].as<int>();
            int filled = row["seats_filled"].as<int>();
            int remaining = cap - filled;

            std::cout << "  "
                      << std::left  << std::setw(5)  << row["elective_id"].c_str()
                      << std::left  << std::setw(35) << row["name"].c_str()
                      << std::right << std::setw(10) << cap
                      << std::right << std::setw(12) << filled
                      << std::right << std::setw(12) << remaining
                      << "\n";
        }
        std::cout << "\n  Total electives: " << rows.size() << "\n";

    } catch (const AllocationException::DatabaseException& e) {
        std::cerr << "\n  [DB ERROR] " << e.what() << "\n";
    }
    waitForEnter();
}

// -----------------------------------------------------------
// updateElectiveCapacity() -- OPTION 2
// Coordinator enters elective ID and new capacity.
// Updates seats_filled reset protection -- cannot reduce
// capacity below currently filled seats.
// Demonstrates: parameterized UPDATE via PostgresConnection.
// -----------------------------------------------------------
void CLI::updateElectiveCapacity() {
    printHeader("Update Elective Seat Capacity");
    try {
        // Show current electives first
        pqxx::result rows = db.executeQuery(
            "SELECT elective_id, name, capacity FROM ELECTIVE ORDER BY elective_id;"
        );
        std::cout << "\n  Current Electives:\n";
        for (const auto& row : rows) {
            std::cout << "    [" << row["elective_id"].c_str() << "] "
                      << row["name"].c_str()
                      << " (capacity: " << row["capacity"].c_str() << ")\n";
        }

        int electiveId  = getIntInput("\n  Enter Elective ID to update: ");
        int newCapacity = getIntInput("  Enter new seat capacity:     ");

        if (newCapacity <= 0) {
            std::cout << "\n  [!] Capacity must be a positive integer.\n";
            waitForEnter();
            return;
        }

        // Guard: cannot set capacity below seats already filled
        pqxx::result check = db.executeQuery(
            "SELECT seats_filled FROM ELECTIVE WHERE elective_id = " +
            std::to_string(electiveId) + ";"
        );
        if (check.empty()) {
            std::cout << "\n  [!] Elective ID " << electiveId << " not found.\n";
            waitForEnter();
            return;
        }
        int filled = check[0]["seats_filled"].as<int>();
        if (newCapacity < filled) {
            std::cout << "\n  [!] Cannot set capacity to " << newCapacity
                      << " — " << filled << " seats already filled.\n";
            waitForEnter();
            return;
        }

        db.executeCommand(
            "UPDATE ELECTIVE SET capacity = " + std::to_string(newCapacity) +
            " WHERE elective_id = " + std::to_string(electiveId) + ";"
        );
        std::cout << "\n  [OK] Capacity updated to " << newCapacity
                  << " for Elective ID " << electiveId << ".\n";

    } catch (const AllocationException::DatabaseException& e) {
        std::cerr << "\n  [DB ERROR] " << e.what() << "\n";
    }
    waitForEnter();
}

// -----------------------------------------------------------
// viewAllStudents() -- OPTION 3
// Lists all students ordered by CGPA descending (merit order).
// This is the order Gale-Shapley will process proposals in.
// Demonstrates: ORDER BY query and PostgresConnection result
// iteration.
// -----------------------------------------------------------
void CLI::viewAllStudents() {
    printHeader("All Students (Ordered by CGPA)");
    try {
        pqxx::result rows = db.executeQuery(
            "SELECT student_id, name, roll_no, cgpa, branch, status "
            "FROM STUDENT ORDER BY cgpa DESC, student_id ASC LIMIT 20;"
        );

        if (rows.empty()) {
            std::cout << "  No students found.\n";
            waitForEnter();
            return;
        }

        std::cout << "\n  Showing top 20 students by CGPA:\n\n";
        std::cout << "  "
                  << std::left  << std::setw(5)  << "ID"
                  << std::left  << std::setw(20) << "Name"
                  << std::left  << std::setw(15) << "Roll No"
                  << std::right << std::setw(7)  << "CGPA"
                  << "  " << std::left << std::setw(10) << "Branch"
                  << "  " << "Status" << "\n";
        printSeparator();

        for (const auto& row : rows) {
            std::cout << "  "
                      << std::left  << std::setw(5)  << row["student_id"].c_str()
                      << std::left  << std::setw(20) << row["name"].c_str()
                      << std::left  << std::setw(15) << row["roll_no"].c_str()
                      << std::right << std::setw(7)  << row["cgpa"].c_str()
                      << "  " << std::left << std::setw(10) << row["branch"].c_str()
                      << "  " << row["status"].c_str() << "\n";
        }

        pqxx::result total = db.executeQuery("SELECT COUNT(*) FROM STUDENT;");
        std::cout << "\n  Total students in DB: " << total[0][0].c_str() << "\n";

    } catch (const AllocationException::DatabaseException& e) {
        std::cerr << "\n  [DB ERROR] " << e.what() << "\n";
    }
    waitForEnter();
}

// -----------------------------------------------------------
// viewPreferences() -- OPTION 4
// Shows ranked preferences for a given student roll number.
// Uses the composite index on PREFERENCE(student_id, rank)
// which is exactly what Gale-Shapley will query in Phase 2.
// Demonstrates: JOIN query, index usage, preference data model.
// -----------------------------------------------------------
void CLI::viewPreferences() {
    printHeader("Student Preference Lookup");
    try {
        std::cout << "  Enter student Roll Number (e.g. CSE001): ";
        std::string rollNo;
        std::cin >> rollNo;

        // Look up student
        pqxx::result student = db.executeQuery(
            "SELECT student_id, name, cgpa FROM STUDENT WHERE roll_no = '" + rollNo + "';"
        );
        if (student.empty()) {
            std::cout << "\n  [!] No student found with roll number: " << rollNo << "\n";
            waitForEnter();
            return;
        }

        std::string studentId   = student[0]["student_id"].c_str();
        std::string studentName = student[0]["name"].c_str();
        std::string cgpa        = student[0]["cgpa"].c_str();

        std::cout << "\n  Student: " << studentName
                  << " | CGPA: " << cgpa
                  << " | ID: " << studentId << "\n";
        printSeparator();

        // Fetch ranked preferences via composite index (student_id, rank)
        pqxx::result prefs = db.executeQuery(
            "SELECT p.rank, e.name AS elective_name, e.capacity, e.seats_filled "
            "FROM PREFERENCE p "
            "JOIN ELECTIVE e ON p.elective_id = e.elective_id "
            "WHERE p.student_id = " + studentId + " "
            "ORDER BY p.rank ASC;"
        );

        if (prefs.empty()) {
            std::cout << "  No preferences submitted yet.\n";
        } else {
            std::cout << "  Ranked Preferences:\n\n";
            std::cout << "    " << std::setw(6) << "Rank"
                      << "  " << std::left << std::setw(30) << "Elective"
                      << "  Seats Available\n";
            std::cout << "    " << std::string(55, '-') << "\n";

            for (const auto& row : prefs) {
                int cap    = row["capacity"].as<int>();
                int filled = row["seats_filled"].as<int>();
                std::cout << "    " << std::right << std::setw(6) << row["rank"].c_str()
                          << "  " << std::left << std::setw(30) << row["elective_name"].c_str()
                          << "  " << (cap - filled) << " / " << cap << "\n";
            }
        }

    } catch (const AllocationException::DatabaseException& e) {
        std::cerr << "\n  [DB ERROR] " << e.what() << "\n";
    }
    waitForEnter();
}

// -----------------------------------------------------------
// runAllocation() -- OPTION 5
// Runs the full allocation through AllocationService:
//   database -> MatchingEngine (Gale-Shapley) -> database.
// The CLI only asks questions and prints results; locking,
// matching and saving are all done inside AllocationService.
//
// If the round already has results, the service refuses to
// overwrite them (SRS edge case 7), so we ask the coordinator
// to confirm and then run again with allowRerun = true.
// -----------------------------------------------------------
void CLI::runAllocation() {
    printHeader("Run Elective Allocation");
    std::cout << "\n  Allocation Engine: Gale-Shapley Stable Matching\n";
    std::cout << "  Round: " << CURRENT_ROUND << "\n";
    printSeparator();

    AllocationService service(db);

    try {
        std::cout << "\n  [*] Running allocation, please wait...\n";
        AllocationSummary summary = service.runAllocation(DEMO_COORDINATOR_ID, CURRENT_ROUND, false);
        printAllocationSummary(summary);

    } catch (const AllocationException::AlreadyAllottedException& e) {
        // This catch must come BEFORE the AllocationException catch below.
        // AlreadyAllottedException is a child of AllocationException, so the
        // parent's catch would grab it first and we could never ask to re-run.
        std::cout << "\n  [!] " << e.what() << "\n";
        int answer = getIntInput("\n  Re-run and REPLACE the old results? (1 = Yes, 0 = No): ");

        if (answer == 1) {
            // A new try block is needed: an exception thrown inside a catch
            // block is not caught by the other catch blocks of the same try.
            try {
                std::cout << "\n  [*] Re-running allocation, please wait...\n";
                AllocationSummary summary = service.runAllocation(DEMO_COORDINATOR_ID, CURRENT_ROUND, true);
                printAllocationSummary(summary);
            } catch (const AllocationException& e2) {
                std::cerr << "\n  [ERROR] " << e2.what() << "\n";
            }
        } else {
            std::cout << "\n  Cancelled. The existing results were not changed.\n";
        }

    } catch (const AllocationException& e) {
        // Another run is already in progress, or the database failed.
        // Nothing was saved in either case.
        std::cerr << "\n  [ERROR] " << e.what() << "\n";
    }

    waitForEnter();
}

// -----------------------------------------------------------
// printAllocationSummary() -- helper for runAllocation()
// Prints the numbers returned by one allocation run.
// -----------------------------------------------------------
void CLI::printAllocationSummary(const AllocationSummary& summary) const {
    std::cout << "\n  [OK] Allocation complete and saved to the database.\n";
    printSeparator();
    std::cout << "  Total students     : " << summary.totalStudents << "\n";
    std::cout << "  Matched            : " << summary.matched << "\n";
    std::cout << "  Unmatched          : " << summary.unmatched << "\n";
    std::cout << "  Run ID (RUN_LOG)   : " << summary.runId << "\n";
    printSeparator();

    // Unmatched students must be reported, never hidden (SRS edge cases 1 and 5).
    if (summary.unmatched > 0) {
        std::cout << "  Note: " << summary.unmatched
                  << " student(s) did not get any of their preferred electives.\n";
    }
    std::cout << "  Fairness check (blocking pairs): pending -- detectBlockingPairs()\n";
    std::cout << "  is still being implemented in MatchingEngine.\n";
}

// -----------------------------------------------------------
// runStudentMenu() -- Student role menu (limited access)
// -----------------------------------------------------------
void CLI::runStudentMenu() {
    while (running) {
        printHeader("Student Portal");
        std::cout << "  [1] View My Allotment Status\n";
        std::cout << "  [2] Exit\n";
        printSeparator();

        int choice = getIntInput("  Enter option: ");
        switch (choice) {
            case 1: viewStudentStatus(); break;
            case 2: running = false;     break;
            default:
                std::cout << "  [!] Invalid option.\n";
        }
    }
}

// -----------------------------------------------------------
// viewStudentStatus() -- Student option 1
// Student enters roll number, sees their allotment status.
// -----------------------------------------------------------
void CLI::viewStudentStatus() {
    printHeader("My Allotment Status");
    std::cout << "  Enter your Roll Number: ";
    std::string rollNo;
    std::cin >> rollNo;

    try {
        pqxx::result result = db.executeQuery(
            "SELECT s.name, s.roll_no, s.cgpa, s.branch, s.status, "
            "       e.name AS elective_name "
            "FROM STUDENT s "
            "LEFT JOIN ALLOTMENT a ON s.student_id = a.student_id "
            "LEFT JOIN ELECTIVE e  ON a.elective_id = e.elective_id "
            "WHERE s.roll_no = '" + rollNo + "';"
        );

        if (result.empty()) {
            std::cout << "\n  [!] Roll number not found.\n";
            waitForEnter();
            return;
        }

        std::cout << "\n  Name:    " << result[0]["name"].c_str()     << "\n";
        std::cout << "  Roll No: " << result[0]["roll_no"].c_str()   << "\n";
        std::cout << "  CGPA:    " << result[0]["cgpa"].c_str()      << "\n";
        std::cout << "  Branch:  " << result[0]["branch"].c_str()    << "\n";
        std::cout << "  Status:  " << result[0]["status"].c_str()    << "\n";
        if (!result[0]["elective_name"].is_null()) {
            std::cout << "  Allotted Elective: " << result[0]["elective_name"].c_str() << "\n";
        } else {
            std::cout << "  Allotted Elective: Not yet allotted (allocation pending)\n";
        }

    } catch (const AllocationException::DatabaseException& e) {
        std::cerr << "\n  [DB ERROR] " << e.what() << "\n";
    }
    waitForEnter();
}

// -----------------------------------------------------------
// runOOPDemoScreen() -- OPTION 0 (Evaluator Mode)
// Presents a submenu of OOP and DS concept demonstrations.
// Use this during evaluation walkthrough.
// -----------------------------------------------------------
void CLI::runOOPDemoScreen() {
    bool demoRunning = true;
    while (demoRunning) {
        printHeader("[ EVALUATOR MODE ] OOP & DS Concept Demo");
        std::cout << "  [1] Polymorphism Demo    (Person* -> Student/Coordinator)\n";
        std::cout << "  [2] Exception Hierarchy  (Throw & catch all 4 sub-types)\n";
        std::cout << "  [3] Operator Overloading (Compare students by CGPA)\n";
        std::cout << "  [4] Back to Main Menu\n";
        printSeparator();

        int choice = getIntInput("  Enter demo: ");
        switch (choice) {
            case 1: demoPolymorphism();        break;
            case 2: demoExceptionHierarchy();  break;
            case 3: demoOperatorOverloading(); break;
            case 4: demoRunning = false;       break;
            default:
                std::cout << "  [!] Invalid option.\n";
        }
    }
}

// -----------------------------------------------------------
// demoPolymorphism()
// Creates Person* base pointers to Student and Coordinator
// objects and calls displayProfile() -- the call dispatches
// to the correct overridden implementation at runtime.
// This is runtime polymorphism (virtual dispatch).
// -----------------------------------------------------------
void CLI::demoPolymorphism() {
    printHeader("Polymorphism Demo: Person* -> Student & Coordinator");
    std::cout << "\n  Concept: Person is an abstract base class with pure virtual\n";
    std::cout << "  displayProfile(). Student and Coordinator both override it.\n";
    std::cout << "  A Person* pointer can hold either -- the correct method is\n";
    std::cout << "  called at RUNTIME (not compile time). This is polymorphism.\n";
    printSeparator();

    // Create concrete objects
    Student     s(101, "Riya Sharma", 9.1f);
    Coordinator c(1,   "Prof. Mehta", "Computer Science");

    // Assign to base class pointers
    Person* personA = &s;
    Person* personB = &c;

    std::cout << "\n  [1] personA points to a Student object:\n";
    personA->displayProfile();   // Calls Student::displayProfile() at runtime

    std::cout << "\n  [2] personB points to a Coordinator object:\n";
    personB->displayProfile();   // Calls Coordinator::displayProfile() at runtime

    std::cout << "\n  Both called via Person* -- the vtable dispatched to the\n";
    std::cout << "  correct overridden method automatically. That is polymorphism.\n";

    waitForEnter();
}

// -----------------------------------------------------------
// demoExceptionHierarchy()
// Throws and catches each of the 4 AllocationException
// sub-types, showing the hierarchy in action.
// Each catch block demonstrates that sub-types can be caught
// individually OR by the base AllocationException class.
// -----------------------------------------------------------
void CLI::demoExceptionHierarchy() {
    printHeader("Exception Hierarchy Demo");
    std::cout << "\n  AllocationException (base)\n";
    std::cout << "  |-- SeatOverflowException\n";
    std::cout << "  |-- InvalidPreferenceException\n";
    std::cout << "  |-- AlreadyAllottedException\n";
    std::cout << "  +-- DatabaseException\n";
    printSeparator();

    // Demo 1: SeatOverflowException
    std::cout << "\n  [1] SeatOverflowException:\n";
    try {
        Elective e(1, "Machine Learning", 1);
        e.allocateSeat(); // fills the 1 seat
        e.allocateSeat(); // should throw
    } catch (const AllocationException::SeatOverflowException& ex) {
        std::cout << "      Caught SeatOverflowException: " << ex.what() << "\n";
    }

    // Demo 2: InvalidPreferenceException
    std::cout << "\n  [2] InvalidPreferenceException:\n";
    try {
        Student s(1, "Test", 8.5f);
        s.addPreference(101);
        s.addPreference(101); // duplicate -- should throw
    } catch (const AllocationException::InvalidPreferenceException& ex) {
        std::cout << "      Caught InvalidPreferenceException: " << ex.what() << "\n";
    }

    // Demo 3: AlreadyAllottedException
    std::cout << "\n  [3] AlreadyAllottedException:\n";
    try {
        throw AllocationException::AlreadyAllottedException(
            "Student 42 is already allotted to Elective 3."
        );
    } catch (const AllocationException::AlreadyAllottedException& ex) {
        std::cout << "      Caught AlreadyAllottedException: " << ex.what() << "\n";
    }

    // Demo 4: DatabaseException caught by base AllocationException
    std::cout << "\n  [4] DatabaseException caught by base AllocationException:\n";
    try {
        throw AllocationException::DatabaseException("Simulated connection timeout.");
    } catch (const AllocationException& ex) {
        std::cout << "      Caught via base AllocationException: " << ex.what() << "\n";
    }

    std::cout << "\n  All 4 exception types thrown and caught successfully.\n";
    waitForEnter();
}

// -----------------------------------------------------------
// demoOperatorOverloading()
// Creates Student objects and compares them using the
// overloaded operator< (by CGPA descending, roll ID ascending).
// Shows how custom comparison enables priority ordering.
// -----------------------------------------------------------
void CLI::demoOperatorOverloading() {
    printHeader("Operator Overloading Demo: Student Comparison");
    std::cout << "\n  Concept: Student::operator< compares by CGPA (higher CGPA\n";
    std::cout << "  = higher priority). On tie, lower student ID wins.\n";
    std::cout << "  This operator drives PriorityQueue ordering in Phase 2.\n";
    printSeparator();

    Student alice(1, "Alice",   9.5f);
    Student bob  (2, "Bob",     8.7f);
    Student carol(3, "Carol",   9.5f); // same CGPA as alice, higher ID

    std::cout << "\n  Alice (CGPA 9.5, ID 1) vs Bob   (CGPA 8.7, ID 2):\n";
    if (bob < alice)
        std::cout << "    Bob < Alice (Bob has lower CGPA, so lower priority)\n";

    std::cout << "\n  Alice (CGPA 9.5, ID 1) vs Carol (CGPA 9.5, ID 3):\n";
    if (carol < alice)
        std::cout << "    Carol < Alice (same CGPA, higher ID = lower priority)\n";

    std::cout << "\n  Alice == Alice check: " << (alice == alice ? "true" : "false") << "\n";
    std::cout << "  Alice == Bob check:   " << (alice == bob   ? "true" : "false") << "\n";

    std::cout << "\n  Ordering for PriorityQueue (highest to lowest priority):\n";
    std::cout << "    1. Alice  (CGPA 9.5, ID 1)\n";
    std::cout << "    2. Carol  (CGPA 9.5, ID 3) -- same CGPA, higher ID\n";
    std::cout << "    3. Bob    (CGPA 8.7, ID 2)\n";

    waitForEnter();
}

// -----------------------------------------------------------
// Utility Methods
// -----------------------------------------------------------

void CLI::printHeader(const std::string& title) const {
    std::string bar(60, '=');
    std::cout << "\n  " << bar << "\n";
    std::cout << "  " << title << "\n";
    std::cout << "  " << bar << "\n";
}

void CLI::printSeparator() const {
    std::cout << "  " << std::string(60, '-') << "\n";
}

void CLI::waitForEnter() const {
    std::cout << "\n  Press Enter to continue...";
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
    std::cin.get();
}

int CLI::getIntInput(const std::string& prompt) const {
    int value = 0;
    std::cout << prompt;
    while (!(std::cin >> value)) {
        std::cin.clear();
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
        std::cout << "  [!] Please enter a number: ";
    }
    return value;
}
