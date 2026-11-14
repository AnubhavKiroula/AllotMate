# CONTEXT — AllotMate

Living project state. Keep concise; update at meaningful milestones.

## What is being built
A C++17 engine that allocates students to electives with the Gale–Shapley
stable matching algorithm, backed by PostgreSQL 16 (via `libpqxx`) and driven by
an interactive CLI. Academic PBL (DS + OOP) at ~1,500-student scale.

## Current objective
Phase 3 (end-semester, 50 marks): final evaluation. All planned features are
built; what remains is the team demo rehearsal and viva prep.

## Architecture (current)
`main.cpp` → `CLI` → repositories / `AllocationService` → `PostgresConnection`
(RAII, `.env`) → PostgreSQL. `AllocationService` runs
DB → `MatchingEngine` (Gale–Shapley) → DB, guarded by an advisory-lock
`RunLock`. Allotments, statuses, seat counts and every elective's waitlist are
saved in ONE transaction, and every run is audited in `RUN_LOG` with its real
blocking-pair count. All non-entrypoint sources build into the `allotmate_core`
library shared by the app, tests and tools.

## Team STL rule (agreed)
`vector`, `string`, streams, smart pointers, `map`, `set` allowed (OOP Unit 5).
DS logic from the lectures is hand-written: no `std::sort`, `<stack>`,
`<queue>`, `std::priority_queue`, `std::list` in project logic; BST/AVL/linked
list/graph from our own nodes; our own `HashTable` instead of `unordered_*`.

## Completed
- Phase 1 (all): schema, Docker, CI, `PostgresConnection`, domain models, CLI skeleton.
- Phase 2 (M1): student/elective repos, CSV import + bulk insert, run log, pool, tests.
- M4 (Dhruv): `SparseMatrix`, `HashTable` (rehash), own-storage `Stack`, `PriorityQueue`,
  Gale–Shapley loop; `countBlockingPairs` / `detectBlockingPairs`, `FairnessReport`
  (first choice %, average rank, blocking pairs), FCFS vs Gale–Shapley benchmark
  (`tools/benchmark_matching.cpp`, `scripts/benchmark_seed.sql`). PR #22, #31.
- M2 (Kritika): `Person` polymorphism, `AVLTree` + `ThreadedBST` (own nodes, Rule of 5),
  AVL waitlist in `Elective`, `MeritRankingReport` (threaded in-order),
  `test_models`, evaluator-mode DS demos in the CLI. PR #32–#41.
- M1 (Anubhav): `AllocationService` + `RunLock`, `WaitlistRepository`, `ReportRepository`,
  merge sort, merge-join `findAll`, 1,500 scale seed, `benchmark_db`, `EXPLAIN`
  profiling, `docs/PERFORMANCE.md`, Phase 3 guide.
- M3 (Gaurang): CLI screens Run Allocation (with fairness report), Results, Waitlist,
  CSV import; 1,500 load test.
- Final phase: waitlists saved after every run (FR-7) and shown to students
  with their position (FR-2); SQL injection in two CLI roll-number queries
  fixed with `quoteLiteral`; CLI exits on end of input instead of looping.

## Open work
- **Team:** demo rehearsal on the demo laptop (fresh `docker compose down -v && up -d`).
- **Team:** decide whether to drop the two redundant indexes (`docs/PERFORMANCE.md`).
- **Deferred:** ML microservice (collusion check / demand forecast); no
  past-semester data to train on.

## Important decisions
- Repositories borrow a `PostgresConnection&`; SQL strings escaped with `quoteLiteral`.
- Allocation results are written in ONE transaction; a re-run of an allocated
  round needs explicit `allowRerun` (SRS #7); concurrent runs refused (SRS #10).
- A run whose stability was not checked stores `NULL` blocking pairs, never a
  fake `0`. Since PR #31 every run stores the real count.
- Waitlist rule: a student waits for every elective ranked above the one they
  got (all choices if unmatched), ordered by the AVL tree (CGPA, then lower ID).
  Stability guarantees everyone inside an elective outranks everyone waiting.
- Optional SQL lives in `scripts/` (Docker auto-runs everything in `sql/`).
- Existing team databases need `scripts/migrate_phase3.sql` once.

## Validation status
- `ctest`: 5/5 suites passing (csv_importer, models, MatchingEngineTest,
  db_layer, allocation) on Ubuntu 24.04 + libpqxx 7.8.1, `-Wall -Wextra -Werror`,
  PostgreSQL 16; ASan/UBSan clean. All 7 DS demos compile in CI.
- `db_layer` and `allocation` also pass on the 1,500-student dataset.
- 1,500 students (`scripts/benchmark_seed.sql`): FCFS leaves 1,336 blocking
  pairs, Gale–Shapley 0, with the same 74% first-choice rate.

## Useful commands
```bash
docker compose up -d
cmake -B build -S . && cmake --build build
ctest --test-dir build --output-on-failure
docker exec -i allotmate_postgres psql -U postgres -d allotmate < scripts/migrate_phase3.sql
docker exec -i allotmate_postgres psql -U postgres -d allotmate -v student_count=1500 < scripts/benchmark_seed.sql
./build/benchmark_matching      # FCFS vs Gale–Shapley on the loaded data
./build/benchmark_db            # DB timings on the scale dataset
```
