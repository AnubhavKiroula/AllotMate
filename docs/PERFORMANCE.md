# Performance — 1,500-Student Scale (Member 3)

Measured numbers from real runs, not estimates. The dataset comes from
`scripts/scale_seed_1500.sql`, the timings from `tools/benchmark_db.cpp`, and the
query plans from `scripts/explain_core_queries.sql`.

## Test setup

| Item | Value |
|---|---|
| Dataset | 1,500 students · 30 electives (1,521 seats, capacities 23–80) · 7,500 preferences (5 each) |
| Database | PostgreSQL 16.15 in Docker (`postgres:16`) |
| Client | Ubuntu 24.04 container, g++ 13.3, libpqxx 7.8.1, Release build |
| Host | AMD Ryzen 7 7435HS (16 threads), 15 GB RAM, Docker Desktop on Windows 11 (WSL2) |
| Network | Client and database containers on the same Docker network (one host) |

Because both containers run on one machine, network latency here is lower than
it would be against a remote database. Round-trip-heavy numbers (the lookup
row) would be slower over a real network.

## Benchmark results (4 runs)

| Step | Range over 4 runs |
|---|---|
| CSV parse + validate + merge sort (1,500 rows) | 3.5 – 4.3 ms |
| `bulkInsert` of 1,500 rows (chunks of 500) | 53 – 66 ms |
| `findAll`: 1,500 students + 7,500 preferences (avg of 5 calls) | 9.9 – 11.9 ms |
| 1,500 separate lookups by `roll_no` (total) | 1,073 – 1,112 ms |
| ↳ per lookup | 0.72 – 0.74 ms |

### What the numbers show
- **Batching beats per-row queries by about 100×.** One indexed lookup takes
  ~0.73 ms end-to-end, but `EXPLAIN ANALYZE` (below) shows the query itself
  executing in **0.048 ms**. So ~93% of each lookup is client↔server round
  trip and transaction overhead, not database work. Loading every student
  and preference with `findAll` (two queries) takes ~10 ms. Doing the same
  work one row at a time would cost seconds.
- That's why `findAll` uses one preference query plus a merge-join instead of
  one query per student (the N+1 problem), and why `bulkInsert` uses multi-row
  `INSERT`s.
- The importer's hand-written merge sort handles 1,500 rows in a few milliseconds, so it's
  nowhere near being the bottleneck.

### Full allocation at 1,500: blocked, not measured
`benchmark_db --allocate` currently fails with **`HashTable is full`**. The
cause is in the matching engine (Member 1, PR #22): `HashTable` has a fixed
capacity of 101 slots and never rehashes, so the engine can't index more than
101 students. The 100-student seed fits only by coincidence. Importing even
8 more students reproduces the failure (verified).

Because the engine throws before any result is saved, and
`AllocationService` writes in a single transaction, the failed run left
**no partial data** in the database. The root cause and a suggested fix
(grow and rehash at load factor 0.7) are on PR #22. The full-run timing will be
added here once that fix lands.

## Index profiling (`EXPLAIN (ANALYZE, BUFFERS)`, 1,500 scale)

| # | Query (where it's used) | Plan | Execution |
|---|---|---|---|
| 1 | `STUDENT WHERE roll_no = ?` (student portal, duplicate check) | Index Scan on `idx_student_roll_no` | 0.048 ms |
| 2 | One student's `PREFERENCE … ORDER BY rank` (`findById`) | Bitmap Index Scan on `idx_preference_student_rank` + Sort (5 rows) | 0.091 ms |
| 3 | All `PREFERENCE … ORDER BY student_id, rank` (`findAll`) | Index Scan on `idx_preference_student_rank`, **no Sort step** | 3.16 ms (7,500 rows) |
| 4 | `ALLOTMENT WHERE elective_id = ?` (capacity, results screen) | Bitmap Index Scan on `idx_allotment_elective_id` | 0.052 ms |

For query 4, `ALLOTMENT` was filled with 1,500 synthetic rows inside a
transaction that the script rolls back.

### Observations
- **All four hot queries use an index at this scale**; none falls back to a
  sequential scan.
- **Query 3 is the important one.** Because the composite index is ordered by
  `(student_id, rank)`, PostgreSQL reads rows already in the order `findAll`
  needs and skips sorting 7,500 rows. That pre-sorted output is exactly what
  makes the merge-join in `findAll` a single linear pass.
- In query 2 the planner chose a bitmap scan plus a tiny in-memory sort of 5 rows
  instead of reading the index in order. For 5 rows the difference is
  negligible (0.09 ms total).

### Finding: two redundant indexes
The index listing shows two pairs of identical B-trees:

| Explicit index | Duplicates the index created by | Columns |
|---|---|---|
| `idx_student_roll_no` | `UNIQUE` constraint `uk_student_roll_no` | `(roll_no)` |
| `idx_preference_student_rank` | `UNIQUE` constraint `uk_preference_student_rank` | `(student_id, rank)` |

PostgreSQL automatically creates a B-tree index for every `UNIQUE` constraint,
so the explicit `CREATE INDEX` lines add a second identical tree. That doubles
the index maintenance on every insert (including `bulkInsert`) with no read
benefit: the planner would use the unique-constraint indexes the same way.

**Recommendation (not applied yet):** drop the two explicit indexes from
`sql/01_init_schema.sql`. This needs team agreement first, because the Phase 1
docs and README name those indexes in the evaluation talking points.

## How to reproduce
```bash
docker exec -i allotmate_postgres psql -U postgres -d allotmate < scripts/scale_seed_1500.sql
./build/benchmark_db
docker exec -i allotmate_postgres psql -U postgres -d allotmate < scripts/explain_core_queries.sql
docker exec -i allotmate_postgres psql -U postgres -d allotmate < sql/02_seed_data.sql   # restore
```
