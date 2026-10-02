# CONTEXT — AllotMate

Living project state. Keep concise; update at meaningful milestones.

## What is being built
A C++17 engine that allocates students to electives using the Gale–Shapley
stable matching algorithm, backed by PostgreSQL 16 (via `libpqxx`) and driven by
an interactive CLI. Academic PBL (DS + OOP) for ~1,500-student scale.

## Current objective
Member 3 (Backend/DB) Phase 2: repository layer, bulk CSV import, run-log
auditing, connection pooling, and isolated tests.

## Architecture (current)
`main.cpp` → `CLI` → `PostgresConnection` (RAII, `.env`) → PostgreSQL.
Domain models (`Person`/`Student`/`Coordinator`/`Elective`) and the generic
`Repository<T>` interface are shared. Phase 2 adds concrete repositories and
import/audit utilities on top of `PostgresConnection`.

## Completed
- Phase 1 (all members): schema + Docker + CI, domain models + exceptions,
  `PostgresConnection`, CLI skeleton, MatchingEngine stubs.
- Phase 2 (Member 3): `PostgresStudentRepo`, `PostgresElectiveRepo`,
  `CsvImporter` + `StudentRecord`, `RunLogRepository`, `ConnectionPool`,
  `PostgresConnection::quoteLiteral`, CTest tests, CI PostgreSQL service.

## In progress / other members (Phase 2)
- Member 1: custom DS (`SparseMatrix`, `HashTable`, `Stack`, `PriorityQueue`) +
  Gale–Shapley loop.
- Member 2: `AVLTree`/`ThreadedBST`, polymorphic profiles, model tests.
- Member 4: full CLI actions, CTest scaffold expansion, ML service scaffold.

## Important decisions
- Repositories borrow a `PostgresConnection&` (reuse, no per-call connect).
- SQL built with `quoteLiteral` escaping (not string concatenation of raw input).
- `Student` domain model has no roll_no/branch, so full rows enter via
  `CsvImporter`→`bulkInsert`; `save()` persists only the model's own columns.
- DB tests skip (exit 0) when no database is reachable, so CTest stays green
  locally; CI runs them against a PostgreSQL service container.

## Known limitations
- `save()` insert path uses placeholder roll_no/branch (model lacks them).
- Local Windows/MinGW cannot link `libpqxx`; full build verified in CI.

## Validation status
- `tests/test_csv_importer.cpp`: passing locally (g++ C++17, warning-clean).
- Repository SQL: verified against a real PostgreSQL 16 container.
- Full `libpqxx` build + `test_db_layer`: verified in CI.

## Useful commands
```bash
docker compose up -d
cmake -B build -S . && cmake --build build
ctest --test-dir build --output-on-failure
```
