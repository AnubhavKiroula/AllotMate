# AllotMate — Fair Elective & Lab-Batch Allocation Engine

[![CI Build Gate](https://github.com/AnubhavKiroula/AllotMate/actions/workflows/ci.yml/badge.svg)](https://github.com/AnubhavKiroula/AllotMate/actions/workflows/ci.yml)
![Language](https://img.shields.io/badge/Language-C%2B%2B17-blue.svg)
![Database](https://img.shields.io/badge/Database-PostgreSQL%2016-336791.svg)
![Container](https://img.shields.io/badge/Docker-Compose-2496ED.svg)
![License](https://img.shields.io/badge/License-MIT-green.svg)

> **Provably Fair Student-Elective Matching System** 
> Combined Data Structures (C/C++) + Object-Oriented Programming (C++) Academic Project (PBL) with real institutional applicability for 1,500+ students.

---

## 📌 Problem & Solution Overview

- **The Problem:** Every semester, 1,500+ students rank elective courses and lab batches against strict seat limits. Existing manual or First-Come-First-Serve (FCFS) allocations reward speed or luck, are vulnerable to human bias, and offer **no stability guarantee** (students and electives may form blocking pairs who both prefer to swap).
- **The Solution:** **AllotMate** adapts the **Gale–Shapley stable matching algorithm** (student-proposing variant) with seat capacities, custom data structures (Sparse Matrix, Priority Queue, AVL Trees, Threaded BST, Hash Tables), an RAII-compliant PostgreSQL persistence layer via `libpqxx`, and an optional decoupled ML forecasting microservice.

---

## 📖 Documentation Quick Links

Comprehensive project documentation is available in the [`/docs`](./docs) directory:
- [`00_PROJECT-OVERVIEW-AND-DIAGRAMS.md`](docs/00_PROJECT-OVERVIEW-AND-DIAGRAMS.md) — UML Class Diagram, ER Diagram, SOLID Mapping & Evaluation Pitch
- [`01_PRD.md`](docs/01_PRD.md) — Product Requirements Document
- [`02_SRS.md`](docs/02_SRS.md) — Software Requirements Specification & Edge Cases
- [`03_ARCHITECTURE.md`](docs/03_ARCHITECTURE.md) — System Architecture, DB & Containerization
- [`04_UIUX.md`](docs/04_UIUX.md) — CLI Workflow & Evaluation Screens
- [`05_DEVELOPMENT.md`](docs/05_DEVELOPMENT.md) — Development Phases, SOLID Rules & Definition of Done
- [`06_VERSION-CONTROL.md`](docs/06_VERSION-CONTROL.md) — Conventional Commits, Branching Strategy & PR Guidelines
- [`07_PHASE-1-EVALUATION-GUIDE.md`](docs/07_PHASE-1-EVALUATION-GUIDE.md) — Phase 1 Evaluation Guide, Member Role Manual & Viva Q&A
- [`08_PHASE-2-EVALUATION-GUIDE.md`](docs/08_PHASE-2-EVALUATION-GUIDE.md) — Phase 2 Evaluation Guide (Backend & Database) & Viva Q&A
- [`09_PHASE-3-EVALUATION-GUIDE.md`](docs/09_PHASE-3-EVALUATION-GUIDE.md) — Phase 3 Evaluation Guide (Integration, Scale & Safety) & Viva Q&A
- [`PERFORMANCE.md`](docs/PERFORMANCE.md) — Measured 1,500-scale benchmarks & `EXPLAIN ANALYZE` index profiling
- [`DB-SETUP.md`](docs/DB-SETUP.md) — Database Quickstart & pgAdmin Setup Guide

---

## ⚡ Key Highlights & Core Features

1. **Provable Fairness via Gale–Shapley Matching:** Guaranteed stable matching with zero blocking pairs. Includes an automated mathematical fairness checker after every run.
2. **Custom Syllabus Data Structures:** Custom implementations of `SparseMatrix`, `HashTable`, `PriorityQueue`, `AVLTree`, `ThreadedBST`, and `Stack` rather than generic standard library shortcuts.
3. **Strict SOLID & DRY OOP Design:** Abstract base class `Person` with derived `Student` & `Coordinator`, repository pattern (`Repository<T>`), RAII resource wrappers, and custom exception hierarchy.
4. **PostgreSQL Relational Persistence:** Direct SQL via `libpqxx` (no heavy ORM overhead), custom connection pooling, B-tree/composite indexes, and containerized Docker setup.
5. **Bulk CSV Ingestion at Scale:** Validated data ingestion supporting 1,500+ records with duplicate roll-number detection.
6. **Automated CI/CD Quality Gate:** GitHub Actions workflow ensuring `-Wall -Wextra -Werror` zero-warning compilation on all branches.

---

## 🛠️ Technology Stack

| Layer | Technology | Role & Purpose |
|---|---|---|
| **Core Language** | C++17 | Algorithmic matching, domain entities, and data structures |
| **Database** | PostgreSQL 16 | Relational persistence, constraints, and composite indexes |
| **Database Connector** | `libpqxx` (C++ client) | Native, low-overhead SQL execution without ORM |
| **Containerization** | Docker & Docker Compose | Containerized database environment across all machines |
| **Build System** | CMake 3.16+ | Multi-platform build and compiler warning gates |
| **CI / Quality Gate** | GitHub Actions | Automated build verification on `main` and `dev` |
| **DB Inspection GUI** | pgAdmin 4 | Visual table inspection and query evaluation |
| **ML Microservice (Optional)** | Python (FastAPI / scikit-learn) | Isolated elective demand forecasting & collusion detection |

---

## 🏛️ Module Ownership & Syllabus Mapping

| Module | Member / Role | Core DS Concepts (DS Syllabus) | Core OOP Concepts (OOP Syllabus) | Evaluator Track |
|---|---|---|---|---|
| **Matching Engine** | **Member 1** (DS / Algo Lead) | Priority Queue, Stack (backtracking), Sparse Matrix | Strategy pattern, Custom exceptions | **DS Evaluator** |
| **Domain Models & Trees** | **Member 2** (OOP Architect) | AVL Tree, Threaded BST, Hashing | Inheritance, Polymorphism, Operator Overloading | **OOP Evaluator** |
| **Persistence & DB Layer** | **Member 3 (Anubhav Kiroula)** | Sorting (bulk import), Indexing structures | RAII, Repository Pattern (`Repository<T>`), Connection Pool | **Joint Evaluator** |
| **Integration, Testing & ML** | **Member 4** (Integration / ML) | Graph/Cluster analysis (collusion check) | Unit testing, Microservice REST isolation | **Panel Review** |

---

## 🗄️ Database Schema & Relational Design

The system persistence is backed by PostgreSQL 16 across 7 relational entities (DDL in [`sql/01_init_schema.sql`](sql/01_init_schema.sql)):

1. **`STUDENT`**: Stores student profiles, unique roll numbers, branches, and CGPA with constraints.
2. **`ELECTIVE`**: Defines course names, semester, seat capacities, and filled seat counts.
3. **`PREFERENCE`**: Ranked student choices with `UNIQUE(student_id, elective_id)` & `UNIQUE(student_id, rank)` to prevent duplicate preference submissions.
4. **`ALLOTMENT`**: Final allocation records per student and round with `UNIQUE(student_id, round)`.
5. **`WAITLIST`**: Ranked waitlist positions per elective backed by `UNIQUE(elective_id, position)`.
6. **`COORDINATOR`**: Academic coordinator profiles with department and bcrypt-hashed credentials.
7. **`RUN_LOG`**: Immutable audit logs of every Gale-Shapley matching run, tracking timestamps, coordinator ID, blocking pairs detected, and outcome summaries.

### Performance Indexing Strategy
- **B-tree Index on `STUDENT(roll_no)`**: O(log N) fast lookup during individual student queries and duplicate check.
- **Composite Index on `PREFERENCE(student_id, rank)`**: Instant O(1)/O(log N) retrieval of student preference order during Gale-Shapley proposal rounds.
- **Index on `ALLOTMENT(elective_id)`**: Rapid capacity enforcement and elective seat lookups at 1,500-student scale.

---

## 🚀 Quickstart & Setup Guide

> 💡 **For Evaluation Demo (September 5th):**  
> Run these 3 simple commands in order in your terminal:
> ```bash
> # 1. Start PostgreSQL 16 DB (seeds 100 students & 6 electives)
> docker compose up -d
> 
> # 2. Build C++ Engine
> cmake -B build -S . && cmake --build build
> 
> # 3. Launch Interactive CLI
> ./build/AllotMate            # On Linux / WSL
> .\build\Debug\AllotMate.exe  # On Windows
> ```

### 1. Prerequisites
- **Docker Desktop** (running)
- **C++17 Compiler** (GCC 9+, Clang 10+, or MSVC 2019+)
- **CMake 3.16+**
- **pgAdmin 4** (optional, for visual inspection)

### 2. Environment Configuration
Copy the template configuration file:
```bash
cp .env.example .env
```
Ensure `.env` contains:
```ini
DB_HOST=localhost
DB_PORT=5433
DB_NAME=allotmate
DB_USER=postgres
DB_PASSWORD=postgres
```

### 3. Start PostgreSQL Database
```bash
docker compose up -d
```
*Automatically brings up PostgreSQL 16 on port `5433`, runs `sql/01_init_schema.sql`, and seeds 100 students and electives from `sql/02_seed_data.sql`.*

### 4. Build and Run the Interactive CLI
```bash
# Configure build with CMake
cmake -B build -S .

# Compile executable
cmake --build build --config Release

# Launch the AllotMate interactive CLI
./build/AllotMate            # On Linux / macOS / WSL
.\build\Debug\AllotMate.exe  # On Windows
```
The program will launch an interactive terminal menu. Choose `[1] Coordinator` to access the full admin menu, or `[2] Student` for status lookup. Press `[0]` from the coordinator menu to enter **Evaluator Mode** for live OOP and DS concept demonstrations.

### 5. Inspect Data via CLI / pgAdmin
- **Direct Terminal SQL:**
  ```bash
  docker exec -it allotmate_postgres psql -U postgres -d allotmate
  ```
- **pgAdmin GUI:** Register Server with Host `localhost`, Port `5433`, Database `allotmate`, Username/Password `postgres`.

---

## 📋 Comprehensive 3-Phase Team Task Tracker

> **Evaluation Schedule:**
> - **Phase 1 Evaluation:** 20 Marks *(September 5th)*
> - **Phase 2 Evaluation:** 30 Marks *(Mid-Semester)*
> - **Phase 3 Evaluation:** 50 Marks *(End-Semester Final PBL Viva)*

---

### 👤 Member 3: Backend & Database Engineer (Anubhav Kiroula)
**Focus:** PostgreSQL Schema, Libpqxx Integration, Repository Layer, Bulk CSV Import at Scale & CI/CD Pipeline.

#### 🔹 Phase 1: Schema Design, Docker & CI/CD Pipeline (20 Marks) — `COMPLETED`
- [x] **CI/CD Build Pipeline:** Created `.github/workflows/ci.yml` compiling with `-Wall -Wextra -Werror` on PRs and pushes to `main`/`dev`.
- [x] **Dockerized PostgreSQL 16:** Created `docker-compose.yml` with named volume persistence (`pgdata`) and auto-init SQL execution.
- [x] **Complete Relational Schema DDL:** Implemented `sql/01_init_schema.sql` covering all 7 ER diagram entities (`STUDENT`, `ELECTIVE`, `PREFERENCE`, `ALLOTMENT`, `WAITLIST`, `COORDINATOR`, `RUN_LOG`) with explicit PK/FK cascading rules and uniqueness constraints.
- [x] **Indexing Architecture:** Implemented B-tree index on `STUDENT(roll_no)`, composite index on `PREFERENCE(student_id, rank)`, and index on `ALLOTMENT(elective_id)`.
- [x] **Realistic Dummy Dataset:** Created `sql/02_seed_data.sql` seeding 100 students across 4 branches, 6 electives, 400 preferences, and coordinator logins with bcrypt hashes.
- [x] **RAII Database Client Wrapper:** Implemented `PostgresConnection` class (`include/PostgresConnection.h`, `src/PostgresConnection.cpp`) with transaction management, RAII lifecycle, and automatic `.env` config loader.
- [x] **Custom Exception Integration:** Added `AllocationException::DatabaseException` to encapsulate DB failure states without silent errors.
- [x] **End-to-End CRUD Verification Demo:** Implemented student insert, query, readback, and cleanup cycle in `src/main.cpp`.
- [x] **Team Setup Documentation:** Authored `docs/DB-SETUP.md` with step-by-step instructions for Docker, pgAdmin, and environment variables.

#### 🔹 Phase 2: Repository Layer, Bulk Import & Isolation Tests (30 Marks) — `COMPLETED`
- [x] **`PostgresStudentRepo` Implementation:** Concrete repository fulfilling `Repository<Student>` (`include/PostgresStudentRepo.h`, `src/PostgresStudentRepo.cpp`) with `save`/`findById`/`findAll` plus preference loading.
- [x] **`PostgresElectiveRepo` Implementation:** Concrete repository fulfilling `Repository<Elective>` (`include/PostgresElectiveRepo.h`, `src/PostgresElectiveRepo.cpp`).
- [x] **High-Performance Bulk CSV Ingestion:** `CsvImporter` validates and de-duplicates rows (sorted-scan duplicate detection); `PostgresStudentRepo::bulkInsert` loads them in chunked multi-row `INSERT`s with `ON CONFLICT` roll-number rejection.
- [x] **`RUN_LOG` Execution Auditing:** `RunLogRepository` (`include/RunLogRepository.h`, `src/RunLogRepository.cpp`) appends audit rows (coordinator ID, blocking pairs, outcome summary) and reads recent runs.
- [x] **Connection Pooling / Reuse:** `ConnectionPool` with an RAII `PooledConnection` lease (`include/ConnectionPool.h`, `src/ConnectionPool.cpp`) reuses connections instead of reconnecting per query.
- [x] **Isolated Repository Unit Tests:** `tests/test_csv_importer.cpp` (no DB) and `tests/test_db_layer.cpp` (uncommitted-transaction rollback protects seed data); wired into CTest and run in CI against a live PostgreSQL service.
- [x] **Zero-Warning Code Quality:** All database-layer code compiles under `-Wall -Wextra -Werror` (verified by the CI build gate).

#### 🔹 Phase 3: Integration, 1,500-Scale Testing & Demo Readiness (50 Marks) — `MOSTLY COMPLETE`
- [x] **MatchingEngine Wire-Up:** `AllocationService` runs DB → `MatchingEngine` → DB, saves results in one transaction and audits each run in `RUN_LOG`. Verified end-to-end on the seed (100/100 matched, no blocking pairs).
- [x] **1,500-Student Scale Benchmarking:** `scripts/scale_seed_1500.sql` + `tools/benchmark_db.cpp`; measured in [`docs/PERFORMANCE.md`](docs/PERFORMANCE.md). A full 1,500-student allocation takes ~190 ms (1,426 matched, 0 blocking pairs). Scale testing found the engine's fixed-size `HashTable` bug, which is now fixed with rehashing.
- [x] **Waitlist Persistence Sync:** `WaitlistRepository` stores and loads waitlist order atomically, ready for the AVL tree's in-order output (Member 2).
- [x] **Concurrency & Re-run Safety:** RAII `RunLock` (PostgreSQL advisory lock) refuses concurrent runs (SRS #10). Re-running an allocated round needs explicit permission (SRS #7). The `CHECK` constraint blocks lowering capacity below filled seats.
- [x] **CLI Reporting Queries:** `ReportRepository`: allotment results (filterable by elective), unmatched students, waitlist view.
- [x] **Index Performance Profiling:** `scripts/explain_core_queries.sql`. All four hot queries use an index at 1,500 scale. Found two redundant indexes (recommendation in `docs/PERFORMANCE.md`).
- [ ] **Live Evaluation Demo Prep:** The demo script is written ([`docs/09_PHASE-3-EVALUATION-GUIDE.md`](docs/09_PHASE-3-EVALUATION-GUIDE.md)); the team rehearsal is still to do.

---

### 👤 Member 2: OOP Architect
**Focus:** Class Hierarchy, Polymorphism, Operator Overloading, Exception Hierarchy, `AVLTree` & `ThreadedBST`.

#### 🔹 Phase 1: Domain Models & Core Interfaces (20 Marks) — `COMPLETED`
- [x] **Abstract Base Class `Person`:** Defined pure virtual `displayProfile()`, protected ID/name, and virtual destructor (`include/Person.h`).
- [x] **Exception Hierarchy:** Implemented full `AllocationException` base class and sub-exceptions (`SeatOverflowException`, `InvalidPreferenceException`, `AlreadyAllottedException`, `DatabaseException`) in `include/AllocationException.h`.
- [x] **Generic Repository Interface:** Defined template `Repository<T>` with `save()`, `findById()`, and `findAll()` (`include/Repository.h`).
- [x] **Domain Entity Classes:** Implemented `Student` (`include/Student.h`, `src/Student.cpp`), `Coordinator` (`include/Coordinator.h`, `src/Coordinator.cpp`), and `Elective` (`include/Elective.h`, `src/Elective.cpp`).
- [x] **Operator Overloading Baseline:** Implemented `operator<` and `operator==` for `Student` (CGPA tie-break) and `Elective`.

#### 🔹 Phase 2: Waitlist Data Structures & Operator Logic (30 Marks) — `IN PROGRESS`
- [ ] **Template `AVLTree<T>` Implementation:** Build self-balancing AVL Tree for priority waitlist management with LL, RR, LR, RL rotations.
- [ ] **Template `ThreadedBST<T>` Implementation:** Implement threaded binary search tree enabling stack-less inorder traversal for reporting.
- [ ] **Polymorphic Profile Display:** Implement full polymorphic rendering in `Student::displayProfile()` and `Coordinator::displayProfile()`.
- [ ] **Model Unit Tests:** Write isolated unit tests for `Student`, `Elective`, and `Coordinator` methods under edge case conditions.
- [ ] **Copy & Move Semantics:** Implement rule of 5 (copy/move constructors and assignment operators) for custom tree structures.

#### 🔹 Phase 3: Waitlist Sync, Optimization & OOP Viva Rehearsal (50 Marks) — `UPCOMING`
- [ ] **AVL Tree Integration with Elective:** Wire `AVLTree` waitlist directly into `Elective` class for dynamic seat promotion.
- [ ] **Threaded BST Inorder Reporting:** Generate formatted fairness reports utilizing stack-less threaded traversal.
- [ ] **OOP Evaluator Walkthrough:** Prepare comprehensive narrative on SOLID design (Liskov Substitution in `Person`, Dependency Inversion in `Repository`, Open/Closed in allocation strategy).

---

### 👤 Member 1: DS / Algorithm Lead
**Focus:** Gale-Shapley Algorithm, `SparseMatrix`, `HashTable`, `Stack` (Backtracking), `PriorityQueue` & Fairness Verification.

#### 🔹 Phase 1: Algorithm Skeleton & Interface Setup (20 Marks) — `COMPLETED`
- [x] **MatchingEngine Skeleton:** Created `MatchingEngine` class header and implementation stubs (`include/MatchingEngine.h`, `src/MatchingEngine.cpp`).
- [x] **Preference Validation Signatures:** Defined validation methods for student rank integrity and capacity bounds.
- [x] **Data Structure Interface Alignment:** Agreed on data structure contracts without relying on standard STL shortcuts.

#### 🔹 Phase 2: Core Data Structures & Gale-Shapley Loop (30 Marks) — `IN PROGRESS`
- [ ] **Custom `SparseMatrix` Implementation:** Implement triplet-based (`row, col, rank`) sparse matrix for compressed preference storage.
- [ ] **Custom `HashTable<K, V>` Implementation:** Build chaining/probing hash table for O(1) student and elective index lookups.
- [ ] **Custom `Stack<Student>` Backtracking:** Implement custom dynamic stack for unmatched student proposals.
- [ ] **Custom `PriorityQueue<Student>`:** Implement binary heap with sift-up/sift-down for elective preference ordering.
- [ ] **Gale–Shapley Allocation Loop:** Implement student-proposing stable matching loop handling seat capacity and unallocated states.

#### 🔹 Phase 3: Blocking-Pair Detection & FCFS Benchmark (50 Marks) — `UPCOMING`
- [ ] **Mathematical Fairness Proof Engine:** Implement `detectBlockingPairs()` reporting 0 blocking pairs on valid runs.
- [ ] **FCFS vs. Stable Matching Benchmark:** Build side-by-side comparison demo showing blocking pairs in FCFS vs. 0 in AllotMate on identical data.
- [ ] **DS Evaluator Walkthrough:** Rehearse mathematical stability proof and custom data structure complexity analysis.

---

### 👤 Member 4: Integration, Testing, Docs & ML
**Focus:** CLI Menu Flows, Unit Testing Harness, Benchmarks, Documentation & Isolated ML Microservice.

#### 🔹 Phase 1: Project Docs, Workflow & CLI Design (20 Marks) — `COMPLETED`
- [x] **Documentation System:** Co-authored PRD, SRS, UI/UX, Architecture, and Version Control documentation in `/docs`.
- [x] **CLI Menu Specifications:** Defined numbered menu flows for coordinator and student roles (`docs/04_UIUX.md`).
- [x] **Git Workflow Adherence:** Established PR Markdown templates and branch conventions.

#### 🔹 Phase 2: CLI Implementation & Test Harness (30 Marks) — `IN PROGRESS`
- [ ] **CLI Admin Tool:** Implement interactive terminal interface covering main menu states (Import, Set Capacities, Run Allocation, View Results, View Waitlist).
- [ ] **Automated CTest Suite:** Scaffold CTest unit testing framework integrated with CMake.
- [ ] **Isolated ML Microservice Scaffold:** Set up Python FastAPI microservice under `/ml-service` with its own Dockerfile and `requirements.txt`.
- [ ] **Demand Forecasting Model:** Implement regression/random-forest demand forecasting endpoint (`POST /forecast/demand`).

#### 🔹 Phase 3: End-to-End System Testing & Demo Polish (50 Marks) — `UPCOMING`
- [ ] **Collusion Detection Endpoint:** Implement clustering algorithm (`POST /forecast/collusion-check`) flagging suspicious identical preference patterns.
- [ ] **System-Wide Load Testing:** Run end-to-end load tests on 1,500 students × 30 electives; log runtime statistics.
- [ ] **Full Team Demo Rehearsal:** Coordinate live presentation flow, backup output recording, and panel Q&A preparation.

---

## 🧭 Next Work — Remarks per Member

> **Team STL rule** (also in `CLAUDE.md`): `vector`, `string`, streams, smart pointers, `map` and `set` are fine (OOP Unit 5). DS logic from the lectures must be **hand-written**: no `std::sort`, `<stack>`, `<queue>`, `std::priority_queue` or `std::list` in project logic, BST/AVL/linked list/graph built from our own nodes, and our own `HashTable` instead of `unordered_map`/`unordered_set`.

### Member 1 — Dhruv Negi (DS / Algorithm)
> ✅ **Already fixed for you:** the `HashTable is full` blocker. `HashTable` now rehashes at load factor 0.7 (commit `6ffdfe0` on PR #22), and allocation runs at 1,500 students. **Read `HashTable::rehash()` before the viva**: be ready to explain why the threshold is 0.7, why every key must be re-hashed (`hash = key % capacity`), and why value pointers are moved rather than copied.

In priority order:
1. **Implement `detectBlockingPairs()` and make it return the count (`int`).** This is the project's "fairness proof".
   - Logic: for every student and every elective they ranked *above* their placement, that elective must be full of students who outrank them; otherwise it's a blocking pair. A working reference is `countBlockingPairs()` in `tests/test_allocation.cpp`. Write your own version inside the engine, using the engine's data structures.
   - Then in `src/AllocationService.cpp`, replace `STABILITY_NOT_CHECKED` with `engine.detectBlockingPairs()`, so `RUN_LOG` records the real count instead of `NULL`.
2. **Implement `generateFairnessReport()`:** average preference rank received, % of students who got their 1st choice, and the unmatched count.
3. **FCFS vs stable-matching benchmark:** run a first-come-first-serve allocation and Gale–Shapley on the same data (`scripts/scale_seed_1500.sql`), and show FCFS produces blocking pairs while ours has 0.
4. **STL rule clean-up:**
   - `MatchingEngine::loadData` uses `std::unordered_set` for duplicate-ID checks; use your own `HashTable::contains()` instead.
   - `Stack` is a thin wrapper over `std::vector`; give it its own storage (a top-index array or linked nodes), so "our stack" is truly ours.
5. **DS viva prep:** complexity of the matching loop (O(N × M × log K)), plus the heap, the sparse matrix and hashing with rehash.

### Member 2 — Kritika Mundepi (OOP Architect)
- `AVLTree<T>` and `ThreadedBST<T>` built from **our own nodes**, not wrapped around `std::map`/`std::set`. BST is the faculty's main DS focus.
- Feed the AVL waitlist's in-order traversal into `WaitlistRepository::replaceWaitlist(electiveId, orderedIds)`; it is already built and tested.
- Model unit tests and rule-of-five for the trees, as planned.

### Member 4 — Gaurang Nauriyal (Integration / ML)
- CLI **"Run Allocation"**: replace `runAllocationStub()` with `AllocationService::runAllocation(coordinatorId, "Round 1", allowRerun)`. On `AlreadyAllottedException`, ask the coordinator to confirm, then call again with `allowRerun = true`.
- CLI **"View allotment results" / "View waitlist"** screens: format the rows from `ReportRepository` (`allotmentResults`, `unmatchedStudents`, `waitlistFor`).
- 1,500 × 30 system load test (`scripts/scale_seed_1500.sql` is ready), and the ML microservice.

### Member 3 — Anubhav Kiroula (Backend / DB) — remaining
- With team agreement, drop the two redundant indexes found by profiling (see `docs/PERFORMANCE.md`).
- Team demo rehearsal.

---

## 📂 Project Directory Structure

```text
AllotMate/
├── .github/
│   └── workflows/
│       └── ci.yml               # Automated CI build gate (-Wall -Wextra -Werror)
├── docs/                        # Complete PBL specification & design documents
│   ├── 00_PROJECT-OVERVIEW-AND-DIAGRAMS.md
│   ├── 01_PRD.md
│   ├── 02_SRS.md
│   ├── 03_ARCHITECTURE.md
│   ├── 04_UIUX.md
│   ├── 05_DEVELOPMENT.md
│   ├── 06_VERSION-CONTROL.md
│   ├── 07_PHASE-1-EVALUATION-GUIDE.md
│   ├── 08_PHASE-2-EVALUATION-GUIDE.md
│   ├── 09_PHASE-3-EVALUATION-GUIDE.md
│   ├── DB-SETUP.md              # Database & pgAdmin setup guide
│   └── PERFORMANCE.md           # Measured 1,500-scale benchmarks & EXPLAIN plans
├── include/                     # C++ header files & interfaces
│   ├── AllocationException.h    # Custom domain & database exception hierarchy
│   ├── AllocationService.h      # End-to-end run: lock, match, save, audit
│   ├── CLI.h                    # Interactive terminal interface
│   ├── ConnectionPool.h         # RAII pool of reusable DB connections
│   ├── Coordinator.h            # Academic coordinator domain model
│   ├── CsvImporter.h            # CSV parse + validation + merge sort + de-dup
│   ├── Elective.h               # Elective course domain model
│   ├── HashTable.h              # Custom open-addressing hash table
│   ├── MatchingEngine.h         # Gale-Shapley stable matching engine
│   ├── Person.h                 # Abstract base class
│   ├── PostgresConnection.h     # RAII PostgreSQL client wrapper
│   ├── PostgresElectiveRepo.h   # Repository<Elective> implementation
│   ├── PostgresStudentRepo.h    # Repository<Student> + bulk insert
│   ├── PriorityQueue.h          # Custom binary max-heap
│   ├── ReportRepository.h       # Result / unmatched / waitlist report queries
│   ├── Repository.h             # Generic Repository<T> template interface
│   ├── RunLogRepository.h       # RUN_LOG audit read/write
│   ├── SparseMatrix.h           # Triplet sparse preference matrix
│   ├── Stack.h                  # Custom stack
│   ├── Student.h                # Student domain model
│   ├── StudentRecord.h          # Plain CSV row (persistence-free)
│   └── WaitlistRepository.h     # Atomic waitlist persistence
├── sql/                         # Auto-run by Docker on first start
│   ├── 01_init_schema.sql       # 7-table DDL, constraints & indexes
│   └── 02_seed_data.sql         # 100-student realistic seed script
├── scripts/                     # Run by hand (NOT auto-run)
│   ├── explain_core_queries.sql # EXPLAIN ANALYZE index profiling
│   ├── migrate_phase3.sql       # Upgrade existing DBs (nullable blocking pairs)
│   └── scale_seed_1500.sql      # 1,500 students x 30 electives dataset
├── src/                         # C++ implementation source files
│   ├── AllocationService.cpp
│   ├── CLI.cpp
│   ├── ConnectionPool.cpp
│   ├── Coordinator.cpp
│   ├── CsvImporter.cpp
│   ├── Elective.cpp
│   ├── MatchingEngine.cpp
│   ├── MatchingEngineTest.cpp   # Engine unit tests (Member 1)
│   ├── PostgresConnection.cpp
│   ├── PostgresElectiveRepo.cpp
│   ├── PostgresStudentRepo.cpp
│   ├── ReportRepository.cpp
│   ├── RunLogRepository.cpp
│   ├── SparseMatrix.cpp
│   ├── Student.cpp
│   ├── WaitlistRepository.cpp
│   └── main.cpp                 # Application entrypoint
├── tests/                       # CTest unit & integration tests
│   ├── CMakeLists.txt
│   ├── test_allocation.cpp      # End-to-end: capacity, stability, locking
│   ├── test_csv_importer.cpp    # Pure-logic: CSV validation, merge sort, capacity
│   └── test_db_layer.cpp        # Repository/DB integration (skips w/o DB)
├── tools/
│   └── benchmark_db.cpp         # 1,500-scale DB timings
├── data/                        # Sample datasets
│   └── students_sample.csv      # Example bulk-import file
├── .env.example                 # Environment configuration template
├── .gitignore                   # Git ignore rules
├── CLAUDE.md                    # Team coding conventions (incl. STL rule)
├── CMakeLists.txt               # CMake build configuration
├── CONTEXT.md                   # Living project state
├── docker-compose.yml           # PostgreSQL 16 container definition
├── LICENSE                      # MIT License
└── README.md                    # Root project overview & task tracker
```

---

## 🛡️ Development & Version Control Standards

- **SOLID Principles:** Single responsibility per module, open for extension via interfaces (`Repository<T>`), Liskov substitutability on `Person`, and dependency inversion.
- **Fail Loud & RAII:** No silent errors or manual memory/connection cleanup. All failures throw `AllocationException` subclasses.
- **Conventional Commits:** All team commits follow `<type>: <description>` format (`feat`, `fix`, `docs`, `build`, `ci`, `refactor`, `test`).
- **Branch-per-Feature:** Code lands via PRs into `main`/`dev` with automated CI verification.

---

## 📄 License
This project is licensed under the [MIT License](LICENSE) — built for academic Data Structures & Object-Oriented Programming PBL evaluation.