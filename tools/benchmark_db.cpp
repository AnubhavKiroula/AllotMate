// ============================================================
// benchmark_db.cpp -- times the database layer at 1,500-student scale
//
// Intended for the scale dataset (scripts/scale_seed_1500.sql):
//   ./build/benchmark_db              import + query timings
//   ./build/benchmark_db --allocate   also times one full allocation run
//
// Only BENCH-prefixed rows / the BENCH-ROUND round are created, and they are
// removed again at the end. --allocate resets student statuses and seat
// counts afterwards, so only use it on the scale database, not on real data.
// ============================================================

#include <chrono>
#include <iostream>
#include <iomanip>
#include <sstream>
#include <string>
#include <vector>

#include "PostgresConnection.h"
#include "PostgresStudentRepo.h"
#include "CsvImporter.h"
#include "AllocationService.h"

using Clock = std::chrono::steady_clock;

static double msSince(Clock::time_point start) {
    return std::chrono::duration<double, std::milli>(Clock::now() - start).count();
}

static void printRow(const std::string& step, double ms) {
    std::cout << "  " << std::left << std::setw(44) << step
              << std::right << std::setw(10) << std::fixed << std::setprecision(2) << ms << " ms\n";
}

static void cleanUp(PostgresConnection& conn, bool allocated) {
    conn.executeCommand("DELETE FROM STUDENT WHERE roll_no LIKE 'BENCH%';");
    if (allocated) {
        conn.executeCommand("DELETE FROM ALLOTMENT WHERE round = 'BENCH-ROUND';");
        conn.executeCommand("UPDATE STUDENT SET status = 'UNASSIGNED';");
        conn.executeCommand("UPDATE ELECTIVE SET seats_filled = 0;");
        conn.executeCommand("DELETE FROM RUN_LOG WHERE outcome_summary LIKE 'BENCH-ROUND%';");
    }
}

int main(int argc, char** argv) {
    const int rows = 1500;
    const int findAllRuns = 5;
    bool withAllocation = (argc > 1 && std::string(argv[1]) == "--allocate");

    PostgresConnection conn;
    PostgresStudentRepo repo(conn);

    pqxx::result size = conn.executeQuery(
        "SELECT (SELECT count(*) FROM STUDENT), (SELECT count(*) FROM PREFERENCE), "
        "(SELECT count(*) FROM ELECTIVE);");
    std::cout << "Dataset: " << size[0][0].as<int>() << " students, "
              << size[0][1].as<int>() << " preferences, "
              << size[0][2].as<int>() << " electives\n\n";

    bool allocated = false;
    try {
        // 1. CSV parse + validation + merge sort of 1,500 generated rows.
        std::stringstream csv;
        csv << "roll_no,name,cgpa,branch\n";
        for (int i = rows; i >= 1; i--) { // reverse order so the sort has work to do
            csv << "BENCH" << std::setw(4) << std::setfill('0') << i << std::setfill(' ')
                << ",Bench Student " << i << "," << (6.0 + (i % 400) / 100.0) << ",CSE\n";
        }
        Clock::time_point t = Clock::now();
        ImportResult parsed = CsvImporter::parseStream(csv);
        printRow("CSV parse + validate + sort (1,500 rows)", msSince(t));

        // 2. Chunked bulk insert of the same rows.
        t = Clock::now();
        int inserted = repo.bulkInsert(parsed.accepted);
        printRow("bulkInsert (" + std::to_string(inserted) + " rows)", msSince(t));
        conn.executeCommand("DELETE FROM STUDENT WHERE roll_no LIKE 'BENCH%';");

        // 3. findAll: every student with preferences attached (average of runs).
        double total = 0;
        size_t loaded = 0;
        for (int run = 0; run < findAllRuns; run++) {
            t = Clock::now();
            loaded = repo.findAll().size();
            total += msSince(t);
        }
        printRow("findAll, avg of " + std::to_string(findAllRuns) + " (" + std::to_string(loaded)
                 + " students)", total / findAllRuns);

        // 4. One indexed lookup per student by roll number.
        pqxx::result rolls = conn.executeQuery("SELECT roll_no FROM STUDENT;");
        t = Clock::now();
        for (const auto& row : rolls) {
            conn.executeQuery("SELECT student_id FROM STUDENT WHERE roll_no = "
                              + conn.quoteLiteral(row[0].as<std::string>()) + ";");
        }
        double lookupMs = msSince(t);
        printRow("lookup by roll_no x " + std::to_string(rolls.size()) + " (total)", lookupMs);
        printRow("  -> per lookup", rolls.empty() ? 0.0 : lookupMs / rolls.size());

        // 5. Optional: one complete allocation run (load + match + save + log).
        if (withAllocation) {
            AllocationService service(conn);
            t = Clock::now();
            AllocationSummary s = service.runAllocation(-1, "BENCH-ROUND", true);
            allocated = true;
            printRow("full allocation run", msSince(t));
            std::cout << "    " << s.matched << " matched, " << s.unmatched << " unmatched of "
                      << s.totalStudents << "\n";
        }
    } catch (const std::exception& e) {
        std::cerr << "Benchmark failed: " << e.what() << "\n";
        cleanUp(conn, allocated);
        return 1;
    }

    cleanUp(conn, allocated);
    return 0;
}
