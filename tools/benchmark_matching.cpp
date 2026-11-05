
#include <iostream>
#include <vector>
#include <algorithm>
#include <chrono>
#include <iomanip>
#include <stdexcept>

#include "Student.h"
#include "Elective.h"
#include "MatchingEngine.h"
#include "PostgresConnection.h"
#include "PostgresStudentRepo.h"
#include "PostgresElectiveRepo.h"

using Clock = std::chrono::steady_clock;

struct BenchmarkResult {
    std::vector<int> assignedElectiveIds;
    int matched = 0;
    int unmatched = 0;
    int firstChoice = 0;
    double averageRank = 0.0;
    double executionTimeMs = 0.0;
};

// FCFS: process students in ascending student ID order.
// Each student receives their highest-ranked elective with an available seat.
BenchmarkResult runFCFS(
    const std::vector<Student>& students,
    std::vector<Elective> electives
) {
    BenchmarkResult result;
    result.assignedElectiveIds.resize(students.size(), -1);

    for (Elective& elective : electives) {
        elective.resetSeats();
    }

    const auto start = Clock::now();

    for (std::size_t i = 0; i < students.size(); ++i) {
        for (int preferredId : students[i].getPreferenceList()) {
            auto elective = std::find_if(
                electives.begin(),
                electives.end(),
                [preferredId](const Elective& e) {
                    return e.getId() == preferredId;
                }
            );

            if (elective != electives.end() && elective->hasSeat()) {
                elective->allocateSeat();
                result.assignedElectiveIds[i] = preferredId;
                break;
            }
        }
    }

    const auto end = Clock::now();

    result.executionTimeMs = std::chrono::duration<double, std::milli>(end - start).count();
    return result;
}

// Calculate common allocation metrics for either algorithm.
void calculateMetrics(
    BenchmarkResult& result,
    const std::vector<Student>& students
) {
    int totalAssignedRank = 0;

    for (std::size_t i = 0; i < students.size(); ++i) {
        const int assignedId = result.assignedElectiveIds[i];

        if (assignedId == -1) {
            ++result.unmatched;
            continue;
        }

        ++result.matched;

        const std::vector<int>& preferences = students[i].getPreferenceList();

        for (std::size_t rank = 0; rank < preferences.size(); ++rank) {
            if (preferences[rank] == assignedId) {
                totalAssignedRank += static_cast<int>(rank) + 1;

                if (rank == 0) {
                    ++result.firstChoice;
                }
                break;
            }
        }
    }

    result.averageRank = result.matched > 0 ? static_cast<double>(totalAssignedRank) / result.matched : 0.0;
}

// Print comparable results.
void printResults(
    const std::string& algorithm,
    const BenchmarkResult& result,
    int totalStudents
) {
    std::cout << "\n" << algorithm << "\n";
    std::cout << "------------------------------\n";

    std::cout << "Execution time: "<< result.executionTimeMs << " ms\n";

    std::cout << "Matched students: "<< result.matched << "\n";

    std::cout << "Unmatched students: "<< result.unmatched << "\n";

    std::cout << "First-choice allocations: "<< result.firstChoice << "\n";

    const double firstChoicePercentage = totalStudents > 0 ? (static_cast<double>(result.firstChoice) / totalStudents) * 100.0 : 0.0;

    std::cout << "First-choice percentage: " << firstChoicePercentage << "%\n";

    std::cout << "Average preference rank: " << result.averageRank << "\n";
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

        // Run FCFS.
        BenchmarkResult fcfsResult = runFCFS(students, electives);
        calculateMetrics(fcfsResult, students);

        // Run Gale-Shapley using the same input data.
        MatchingEngine engine;
        engine.loadData(students, electives);

        const auto gsStart = Clock::now();
        engine.runGaleShapley();
        const auto gsEnd = Clock::now();

        BenchmarkResult gsResult;
        gsResult.assignedElectiveIds.resize(students.size(), -1);

        gsResult.executionTimeMs = std::chrono::duration<double, std::milli>(gsEnd - gsStart).count();

        for (std::size_t i = 0; i < students.size(); ++i) {
            gsResult.assignedElectiveIds[i] = engine.getAssignedElectiveId(students[i].getId());
        }

        calculateMetrics(gsResult, students);

        // Display both results.
        std::cout << std::fixed << std::setprecision(2);

        printResults("FCFS", fcfsResult, static_cast<int>(students.size()));

        printResults("Gale-Shapley", gsResult, static_cast<int>(students.size()));

        std::cout << "\n====================================\n";
        std::cout << " Benchmark Comparison Complete\n";
        std::cout << "====================================\n";

    } catch (const std::exception& e) {
        std::cerr << "\nBenchmark failed: " << e.what() << "\n";
        return 1;
    }
    return 0;
}
