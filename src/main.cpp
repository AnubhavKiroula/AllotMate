// ============================================================
// main.cpp -- AllotMate Entry Point
//
// Instantiates the CLI and hands control over to it.
// All error handling for startup failures (DB not running,
// bad .env credentials) is caught here and printed clearly.
//
// To run:
//   1. docker compose up -d       (start PostgreSQL)
//   2. cmake -B build -S .        (configure)
//   3. cmake --build build        (compile)
//   4. .\build\Debug\AllotMate.exe  (Windows)
//      ./build/AllotMate             (Linux)
// ============================================================

#include <iostream>
#include "CLI.h"
#include "AllocationException.h"

int main() {
    try {
        CLI cli;
        cli.run();
        return 0;

    } catch (const AllocationException::DatabaseException& e) {
        std::cerr << "\n[FATAL] Database connection failed: " << e.what() << "\n";
        std::cerr << "Hint: Run 'docker compose up -d' and verify your .env file.\n";
        return 1;

    } catch (const std::exception& e) {
        std::cerr << "\n[FATAL] Unexpected error: " << e.what() << "\n";
        return 1;
    }
}
