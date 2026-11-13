#pragma once

#include <string>
#include "PostgresConnection.h"
#include "Student.h"
#include "Coordinator.h"
#include "Elective.h"
#include "AllocationService.h"
#include "AllocationException.h"
#include "ReportRepository.h"
#include "CsvImporter.h"
#include "PostgresStudentRepo.h"

// ============================================================
// CLI -- Interactive Terminal Interface for AllotMate
//
// Responsibilities:
//   - Display a numbered menu to coordinators and students
//   - Route user choices to the correct action handlers
//   - Read live data from PostgreSQL via PostgresConnection
//   - Demonstrate OOP concepts (polymorphism, exceptions, operators)
//     interactively during evaluation
//
// Design: Single Responsibility -- CLI only handles I/O and
// delegates all domain operations to domain model methods and
// the PostgresConnection layer. It does NOT contain business logic.
// ============================================================
class CLI {
private:
    PostgresConnection db;
    bool running;

    // --- Role Selection ---
    // Returns 1 (Coordinator) or 2 (Student)
    int selectRole();

    // --- Coordinator Menu ---
    void runCoordinatorMenu();
    void viewElectives();           // Option 1: List electives + seat counts from DB
    void updateElectiveCapacity();  // Option 2: Set seat capacity for an elective
    void viewAllStudents();         // Option 3: List all students with CGPA from DB
    void viewPreferences();         // Option 4: View ranked preferences per student
    void runAllocation();           // Option 5: Run Gale-Shapley via AllocationService
    void printAllocationSummary(const AllocationSummary& summary) const;

    // --- Report Screens ---
    void viewAllotmentResults();    // Option 6: Allotments per elective + students not allotted
    void viewWaitlist();            // Option 7: Students waiting for a seat in one elective
    void printStudentTableHeader() const;
    void printStudentLine(const std::string& rollNo, const std::string& name,
                          double cgpa, const std::string& branch) const;

    // --- Data Import ---
    void importStudentsFromCsv();   // Option 8: Validate a student CSV, then insert after confirmation

    // --- Student Menu ---
    void runStudentMenu();
    void viewStudentStatus();       // Student: look up own allotment status by roll number

    // --- OOP Demonstration Screens (for evaluator walkthrough) ---
    // Accessible via option 0 from coordinator menu
    void runOOPDemoScreen();
    void demoPolymorphism();        // Show Person* dispatching to Student & Coordinator
    void demoExceptionHierarchy();  // Throw and catch all 4 AllocationException sub-types
    void demoOperatorOverloading(); // Compare two students by CGPA using operator<
    void demoMeritRanking();        // ThreadedBST: rank live students by CGPA without a stack
    void demoAvlWaitlist();         // AVLTree: waitlist promotes the highest-priority student

    // --- Utility ---
    void printHeader(const std::string& title) const;
    void printSeparator() const;
    void waitForEnter() const;
    int  getIntInput(const std::string& prompt) const;

public:
    // Constructor connects to DB via .env -- throws DatabaseException on failure
    CLI();

    // Main entry point -- call from main()
    void run();
};
