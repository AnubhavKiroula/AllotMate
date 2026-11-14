// ============================================================
// benchmark_matching.cpp -- FCFS vs Gale-Shapley on the same data
//
// Loads students and electives from the database, allocates them
// twice (first-come-first-served and Gale-Shapley) and prints one
// comparison table. Both results are judged by the same
// MatchingEngine::buildFairnessReport(), so the blocking-pair
// counts are directly comparable (PRD section 7).
//
// Load a dataset first, e.g. scripts/benchmark_seed.sql.
// Not a test: timings depend on the machine.
// ============================================================

#include <chrono>
#include <iomanip>
#include <iostream>
#include <string>
#include <vector>

#include "Elective.h"
#include "HashTable.h"
#include "MatchingEngine.h"
#include "PostgresConnection.h"
#include "PostgresElectiveRepo.h"
#include "PostgresStudentRepo.h"
#include "Student.h"

using Clock = std::chrono::steady_clock;

struct BenchmarkResult {
    std::vector<int> assignedElectiveIds; // same order as students, -1 = unmatched
    double executionTimeMs = 0.0;
    FairnessReport fairness;
};

// FCFS: process students in ascending student ID order.
// Each student receives their highest-ranked elective with an available seat.
BenchmarkResult runFCFS(
    const std::vector<Student>& students,
    std::vector<Elective> electives
) {
    BenchmarkResult result;
    result.assignedElectiveIds.resize(students.size(), -1);

    // Elective id -> position in the electives vector.
    HashTable<int, int> positionOf;
    for (std::size_t i = 0; i < electives.size(); ++i) {
        electives[i].resetSeats();
        positionOf.insert(electives[i].getId(), static_cast<int>(i));
    }

    const auto start = Clock::now();

    for (std::size_t i = 0; i < students.size(); ++i) {
        for (int preferredId : students[i].getPreferenceList()) {
            Elective& elective = electives[positionOf.find(preferredId)];

            if (elective.hasSeat()) {
                elective.allocateSeat();
                result.assignedElectiveIds[i] = preferredId;
                break;
            }
        }
    }

    const auto end = Clock::now();

    result.executionTimeMs = std::chrono::duration<double, std::milli>(end - start).count();
    return result;
}

// Gale-Shapley through the real engine. Only runGaleShapley() is timed,
// like the FCFS loop above (building the indexes is not timed in either).
BenchmarkResult runGaleShapley(
    const std::vector<Student>& students,
    const std::vector<Elective>& electives
) {
    MatchingEngine engine;
    engine.loadData(students, electives);

    const auto start = Clock::now();
    engine.runGaleShapley();
    const auto end = Clock::now();

    BenchmarkResult result;
    result.assignedElectiveIds = engine.getAssignedElectiveIds();
    result.executionTimeMs = std::chrono::duration<double, std::milli>(end - start).count();
    return result;
}

// One row of the comparison table (counts and decimals are printed differently).
void printRow(const std::string& label, int fcfs, int galeShapley) {
    std::cout << "  " << std::left << std::setw(28) << label
              << std::right << std::setw(10) << fcfs
              << std::setw(15) << galeShapley << "\n";
}

void printRow(const std::string& label, double fcfs, double galeShapley) {
    std::cout << "  " << std::left << std::setw(28) << label
              << std::right << std::setw(10) << fcfs
              << std::setw(15) << galeShapley << "\n";
}

int main() {
    try {
        std::cout << "====================================\n";
        std::cout << " FCFS vs Gale-Shapley Benchmark\n";
        std::cout << "====================================\n";

        // Load identical input data for both algorithms.
        PostgresConnection db;
        PostgresStudentRepo studentRepo(db);
        PostgresElectiveRepo electiveRepo(db);

        const std::vector<Student> students = studentRepo.findAll();
        const std::vector<Elective> electives = electiveRepo.findAll();

        if (students.empty()) {
            std::cout << "No students found. Benchmark cancelled.\n";
            return 0;
        }

        std::cout << "\nDataset loaded successfully.\n";
        std::cout << "Students: " << students.size() << "\n";
        std::cout << "Electives: " << electives.size() << "\n";

        BenchmarkResult fcfs = runFCFS(students, electives);
        BenchmarkResult galeShapley = runGaleShapley(students, electives);

        // Judge both assignments with exactly the same rules.
        fcfs.fairness = MatchingEngine::buildFairnessReport(
            students, electives, fcfs.assignedElectiveIds);
        galeShapley.fairness = MatchingEngine::buildFairnessReport(
            students, electives, galeShapley.assignedElectiveIds);

        const FairnessReport& f = fcfs.fairness;
        const FairnessReport& g = galeShapley.fairness;

        std::cout << std::fixed << std::setprecision(2);
        std::cout << "\n  " << std::left << std::setw(28) << "Metric"
                  << std::right << std::setw(10) << "FCFS"
                  << std::setw(15) << "Gale-Shapley" << "\n";
        std::cout << "  " << std::string(53, '-') << "\n";
        printRow("Execution time (ms)", fcfs.executionTimeMs, galeShapley.executionTimeMs);
        printRow("Matched students", f.matched, g.matched);
        printRow("Unmatched students", f.unmatched, g.unmatched);
        printRow("First-choice allocations", f.firstChoice, g.firstChoice);
        printRow("First-choice percentage", f.firstChoicePercent, g.firstChoicePercent);
        printRow("Average preference rank", f.averageRank, g.averageRank);
        printRow("Blocking pairs", f.blockingPairs, g.blockingPairs);

        std::cout << "\nBlocking pairs: FCFS = " << f.blockingPairs
                  << ", Gale-Shapley = " << g.blockingPairs << "\n";
        std::cout << "A blocking pair is a student and an elective that would both rather\n";
        std::cout << "be matched to each other: a justified complaint against the result.\n";

        std::cout << "\n====================================\n";
        std::cout << " Benchmark Comparison Complete\n";
        std::cout << "====================================\n";

    } catch (const std::exception& e) {
        std::cerr << "\nBenchmark failed: " << e.what() << "\n";
        return 1;
    }
    return 0;
}
