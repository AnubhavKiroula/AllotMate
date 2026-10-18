# CONTEXT — AllotMate

Living project state. Keep concise; update at meaningful milestones.

## What is being built
A C++17 engine that allocates students to electives with the Gale–Shapley
stable matching algorithm, backed by PostgreSQL 16 (via `libpqxx`) and driven by
an interactive CLI. Academic PBL (DS + OOP) at ~1,500-student scale.

## Current objective
Phase 3 (end-semester, 50 marks): end-to-end integration, scale, demo readiness.

## Architecture (current)
`main.cpp` → `CLI` → repositories / `AllocationService` → `PostgresConnection`
(RAII, `.env`) → PostgreSQL. `AllocationService` runs
DB → `MatchingEngine` (Gale–Shapley) → DB in one transaction, guarded by an
advisory-lock `RunLock`, and audits every run in `RUN_LOG`. All non-entrypoint
sources build into the `allotmate_core` library shared by the app, tests and tools.

## Team STL rule (agreed)
`vector`, `string`, streams, smart pointers, `map`, `set` allowed (OOP Unit 5).
DS logic from the lectures is hand-written: no `std::sort`, `<stack>`,
`<queue>`, `std::priority_queue`, `std::list` in project logic; BST/AVL/linked
list/graph from our own nodes; our own `HashTable` instead of `unordered_*`.

## Completed
- Phase 1 (all): schema, Docker, CI, `PostgresConnection`, domain models, CLI skeleton.
- Phase 2 (M3): student/elective repos, CSV import + bulk insert, run log, pool, tests.
- Phase 2/3 (M1, PR #22): `SparseMatrix`, `HashTable`, `Stack`, `PriorityQueue`,
  Gale–Shapley loop. Merge conflicts and three build-blockers fixed by M3.
- Phase 3 (M3): `AllocationService` + `RunLock`, `WaitlistRepository`,
  `ReportRepository`, merge sort (replaces `std::sort`), merge-join `findAll`,
  nullable `RUN_LOG.blocking_pairs_found`, 1,500 scale seed, `benchmark_db`,
  `EXPLAIN` profiling, `docs/PERFORMANCE.md`, Phase 3 guide.

## Open blockers / next work (details in README "Next Work")
- **M1:** `HashTable` is fixed at 101 slots with no rehash, so allocation fails above
  101 students (also after a small CSV import). Fix with rehash at load factor 0.7.
- **M1:** `detectBlockingPairs()` should return a count (RUN_LOG stores NULL until then).
- **M2:** `AVLTree` / `ThreadedBST` from own nodes; feed `WaitlistRepository`.
- **M4:** CLI "Run Allocation" / results / waitlist screens using
  `AllocationService` and `ReportRepository`.
- **Team:** decide whether to drop the two redundant indexes (`docs/PERFORMANCE.md`).

## Important decisions
- Repositories borrow a `PostgresConnection&`; SQL strings escaped with `quoteLiteral`.
- Allocation results are written in ONE transaction; a re-run of an allocated
  round needs explicit `allowRerun` (SRS #7); concurrent runs refused (SRS #10).
- An unchecked run stores `NULL` blocking pairs, never a fake `0`.
- Optional SQL lives in `scripts/` (Docker auto-runs everything in `sql/`).
- Existing team databases need `scripts/migrate_phase3.sql` once.

## Validation status
- `ctest`: 4/4 suites passing (csv_importer, MatchingEngineTest, db_layer,
  allocation) on Ubuntu 24.04 + libpqxx 7.8.1, `-Wall -Wextra -Werror`, PostgreSQL 16.
- Allocation verified on the 100-student seed: 100/100 matched, 0 blocking
  pairs (oracle proven to detect a planted blocking pair).
- 1,500 scale: import/query timings measured; full allocation blocked by HashTable.

## Useful commands
```bash
docker compose up -d
cmake -B build -S . && cmake --build build
ctest --test-dir build --output-on-failure
docker exec -i allotmate_postgres psql -U postgres -d allotmate < scripts/migrate_phase3.sql
./build/benchmark_db            # on the scale dataset (scripts/scale_seed_1500.sql)
```
