# AllotMate — Phase 3 Evaluation Guide (Backend, Integration & Scale)

> **Scope:** Phase 3 is the 50-mark end-semester stage. This guide covers what
> **Member 3 (Anubhav Kiroula — Backend & Database Lead)** built in Phase 3:
> every file, every function, the concept behind it, and the questions an
> evaluator is likely to ask. Measured numbers live in `docs/PERFORMANCE.md`.
> Work still owed by other members is listed in `README.md` under
> "Next work — remarks per member".

---

## 1. What Phase 3 had to achieve (backend side)

By the end of Phase 2 we had repositories that could load and save students and
electives, but nothing actually *ran an allocation* against the database.
Phase 3 closes that loop:

1. **Wire the matching engine to the database.** Load from PostgreSQL, run
   Gale–Shapley, save the result back.
2. **Make re-runs and concurrency safe.** SRS edge case #7 says results are
   never silently overwritten; #10 says two runs at the same time are blocked at the DB
   level.
3. **Persist waitlists** and **provide the report queries** behind the CLI's
   results and waitlist screens.
4. **Prove it scales** to 1,500 students, and **profile the indexes** with real
   `EXPLAIN ANALYZE` output.
5. **Clean up STL usage**, so every DS concept we claim (sorting especially) is
   our own code.

Along the way I integrated Member 1's engine (PR #22). That turned up two real bugs,
both described honestly in section 5.

---

## 2. The end-to-end flow

```
 CLI (Member 4)                       AllocationService::runAllocation
      |                                         |
      |  "Run Allocation"   1. RunLock  --------+--> pg_try_advisory_lock(7310)
      +-------------------> 2. re-run check ----+--> ALLOTMENT has this round?
                            3. load ------------+--> PostgresStudentRepo::findAll
                                                |    PostgresElectiveRepo::findAll
                            4. match -----------+--> MatchingEngine (Member 1)
                            5. save (1 txn) ----+--> ALLOTMENT, STUDENT, ELECTIVE
                            6. audit -----------+--> RunLogRepository::logRun
                            ~RunLock -----------+--> pg_advisory_unlock(7310)

 CLI results / waitlist screens  ------------> ReportRepository (read-only SQL)
 AVL waitlist (Member 2) in-order  ----------> WaitlistRepository (store order)
```

---

## 3. Member 3 — files, functions, and the concept behind each

### 3.1 `include/AllocationService.h` + `src/AllocationService.cpp`

The piece that turns the project from "parts" into "a system".

**`AllocationSummary`** is what a run reports back: total students, matched,
unmatched, and the `RUN_LOG` id. Unmatched students are counted and returned,
never quietly dropped (SRS edge cases #1 and #5).

**`RunLock`** is a small RAII guard for the "one allocation at a time" rule.
- *Constructor:* runs `SELECT pg_try_advisory_lock(7310)`. A PostgreSQL
  **advisory lock** is a named lock the database manages for us: every run asks
  for lock number 7310, and PostgreSQL hands it to only one session at a time. If
  it's already taken, the constructor throws `AllocationException` ("another
  allocation run is already in progress").
- *Destructor:* runs `pg_advisory_unlock(7310)`. Because it's a destructor, the
  lock is released **even if the run throws halfway**. The destructor itself
  never throws; if the connection is already dead, PostgreSQL drops session
  locks on its own.
- Copying is deleted, since a lock must have exactly one owner.

> **Concept:** RAII again, the same idea as `PostgresConnection` and
> `PooledConnection`. Why an advisory lock and not `LOCK TABLE`? A table lock
> would also block the CLI's normal reads while a run is going. The advisory lock
> only blocks *other allocation runs*, which is exactly the rule in SRS #10.

**`AllocationService`** (constructor borrows the `PostgresConnection&`):
- **`roundAlreadyAllocated(round)`** counts `ALLOTMENT` rows for that round.
- **`runAllocation(coordinatorId, round, allowRerun)`** is the whole pipeline:
  take the `RunLock`; if the round is already allocated and `allowRerun` is false, throw
  `AlreadyAllottedException` (the coordinator must confirm a re-run explicitly,
  SRS #7); load students and electives through the repositories; call
  `engine.loadData()` and `engine.runGaleShapley()`; save; log.
- **`saveResults(engine, round)`** asks the engine where each student landed
  (`getAssignedElectiveId`, `-1` = unmatched), builds one multi-row `INSERT`, and
  runs **everything inside one `pqxx::work` transaction**: delete the round's old
  allotments, insert the new ones, reset and set student statuses, and recount
  `seats_filled` from the saved rows. Either all of it commits or none of it does,
  so a failure can never leave half an allocation in the database.
- **`RUN_LOCK_KEY`** is the shared lock number (any fixed number works, as long as
  every run uses the same one).

> **Concepts:** transactions and atomicity, idempotent re-runs (deleting the
> round first makes a re-run *replace* instead of *duplicate*), and recounting
> `seats_filled` from `ALLOTMENT` so the two can never drift apart. The
> `CHECK (seats_filled <= capacity)` constraint stays as a final safety net.

### 3.2 `include/WaitlistRepository.h` + `src/WaitlistRepository.cpp`

- **`replaceWaitlist(electiveId, orderedStudentIds)`** replaces an elective's whole
  waitlist: position 1 is index 0 of the list. The delete and insert run in **one
  transaction**, so if anything is invalid (say, the same student twice, which
  breaks `UNIQUE(student_id, elective_id)`), the old waitlist stays exactly as it
  was. Database errors come back as `DatabaseException`.
- **`loadWaitlist(electiveId)`** returns the ids front of the line first
  (`ORDER BY position`).

> **Concept:** separation of responsibilities. The *order* of a waitlist is
> decided by Member 2's AVL tree (its in-order traversal). This class only
> stores and loads that order. I didn't write a second ranking here, so the
> AVL tree stays the single place where waitlist ordering lives.

### 3.3 `include/ReportRepository.h` + `src/ReportRepository.cpp`

Read-only SQL behind the CLI screens, so the CLI only formats rows:
- **`allotmentResults(round, electiveId = 0)`** joins `ALLOTMENT`, `STUDENT` and
  `ELECTIVE`, sorted by elective and then best CGPA. Pass an elective id to filter
  (the UI/UX doc asks for "filter by elective").
- **`unmatchedStudents(round)`** lists students with **no** allotment that round,
  using `NOT EXISTS`. It exists because the SRS says unmatched students must be
  reported clearly.
- **`waitlistFor(electiveId)`** is an elective's queue with roll numbers and CGPAs.
- `AllotmentRow`, `StudentRow` and `WaitlistRow` are the plain row structs these
  return.

### 3.4 `src/CsvImporter.cpp`: hand-written merge sort (replaces `std::sort`)

- **`sortByRollNo(rows)`** allocates one scratch array and calls `mergeSort`.
- **`mergeSort(rows, temp, left, right)`** works on the half-open range
  `[left, right)`: split at the middle, sort each half recursively, then merge.
  A range of 0 or 1 rows is already sorted, which is the base case.
- **`merge(rows, temp, left, mid, right)`** walks both halves with two indexes,
  always taking the smaller roll number, then copies the leftovers and writes
  the merged run back.

> **Concept: stability.** On a tie, the strict `<` takes the left element
> first, so equal roll numbers keep their original file order. **This actually
> fixed a bug:** Phase 2 used `std::sort`, which is *not* stable, so "keep the
> first duplicate" was never guaranteed. A test now proves the first
> occurrence wins. Complexity: O(n log n) time, O(n) extra space.

### 3.5 `src/PostgresStudentRepo.cpp`: `findAll` merge-join (replaces `std::unordered_map`)

Students come back ordered by `student_id`, and preferences ordered by
`(student_id, rank)`. One index `s` walks the students while the loop walks the
preferences. For each preference row we advance `s` until the ids match. That's
the merge step of merge sort applied to two sorted lists: O(students + preferences),
and no lookup table at all. `EXPLAIN` shows the composite index hands PostgreSQL
the rows already sorted, so the database doesn't sort 7,500 rows either.

### 3.6 `src/RunLogRepository.cpp` + schema: "not checked" is not 0

`detectBlockingPairs()` (Member 1) doesn't return a count yet. Writing `0` would
claim every run was proven stable, which would be a false audit record. So:
- `RUN_LOG.blocking_pairs_found` is now **nullable** (`sql/01_init_schema.sql`).
  `NULL` means "the check was not run".
- `logRun` stores a negative count as `NULL`; `recentRuns` reads `NULL` back as
  `-1`. This follows the same convention as `coordinatorId`.
- `scripts/migrate_phase3.sql` applies the change to databases created
  before it. I tested it against an old-schema database.

### 3.7 Scale and profiling scripts (in `scripts/`, not `sql/`)

Every `.sql` file in `sql/` is **auto-run by Docker** on first start, so
anything optional goes in `scripts/`:
- **`scale_seed_1500.sql`** creates 1,500 students, 30 electives (capacities 23–80, 1,521 seats)
  and 5 preferences each. The offsets 0, 6, 12, 18 and 24 are distinct mod 30, so no student
  ranks an elective twice.
- **`explain_core_queries.sql`** runs `EXPLAIN (ANALYZE, BUFFERS)` on the four hot
  queries. It fills `ALLOTMENT` with synthetic rows inside a transaction it rolls back.
- **`migrate_phase3.sql`** is described in 3.6.

### 3.8 `tools/benchmark_db.cpp`

- **`msSince(start)`** returns elapsed milliseconds from `std::chrono::steady_clock`.
- **`printRow(step, ms)`** prints one aligned result line.
- **`cleanUp(conn, allocated)`** removes `BENCH` rows (and the benchmark round, if run).
- **`main`** times CSV parse + sort, `bulkInsert`, `findAll` (averaged over 5),
  1,500 single lookups, and with `--allocate` a full allocation run.

### 3.9 Tests

- **`tests/test_allocation.cpp`** runs the **real pipeline** on seed data and checks
  the *properties* of the saved result:
  - capacity: no elective over capacity, and `seats_filled` matches;
  - uniqueness and validity: each student is placed in one of their own preferences;
  - status consistency;
  - **stability**, via `countBlockingPairs`;
  - re-run refusal, then an explicit re-run giving identical results (determinism);
  - concurrent-run refusal while another session holds the lock.

  `countBlockingPairs` is a test-side oracle. For each student and each elective
  they ranked above their own placement, that elective must be full of students
  who outrank them; otherwise it's a blocking pair.
  **`testOracleDetectsABlockingPair`** deliberately breaks a stable result and
  checks the oracle catches it. Without that, a checker that always said "0"
  would also pass. Helpers: `loadAllotments`, `rankOf`, `restoreSeedState`. The
  test refuses to run if the database holds real allotments, and it restores
  the seed afterwards.
- **`tests/test_db_layer.cpp`** adds tests for waitlist order and rollback, the report
  queries, NULL run-log semantics, and a check that `findAll` attaches **every**
  preference row.
- **`tests/test_csv_importer.cpp`** adds merge sort edge cases (empty, single,
  reversed, already sorted) and the first-duplicate-wins stability test.

---

## 4. Live demo flow (backend)

```bash
docker compose up -d
cmake -B build -S . && cmake --build build
ctest --test-dir build --output-on-failure
```
1. **Tests:** point at `allocation` passing. The run is checked for capacity,
   stability, re-run refusal and locking, all read back from PostgreSQL.
2. **pgAdmin:** open `RUN_LOG`. Each run is an immutable row, and
   `blocking_pairs_found` is `NULL` because the stability check isn't run yet.
   Be ready to explain why that is more honest than `0`.
3. **Locking:** `testConcurrentRunIsRefused` in the `allocation` test output shows
   it automatically: a second session holds the lock, the run is refused and
   writes nothing, then succeeds once the lock is released. Once Member 4 wires
   "Run Allocation" into the CLI, it can be shown live: hold the lock in `psql`
   with `SELECT pg_advisory_lock(7310);`, press Run, and it's refused.
4. **Scale:** load `scripts/scale_seed_1500.sql`, run `./build/benchmark_db`, then
   `scripts/explain_core_queries.sql`, and show the Index Scan lines. Restore with
   `sql/02_seed_data.sql`.

---

## 5. Honest status and findings

- **Integration:** `AllocationService` works end-to-end on the seed data
  (100/100 matched; the two smallest electives fill exactly to capacity, 15/15 and 10/10).
- **Scale: works at 1,500.** A full allocation takes ~190 ms (1,426 matched,
  74 unmatched, identical every run), and `test_allocation` passes all 18
  checks on the 1,500-student database, including **0 blocking pairs**.
- **Bug found at scale, then fixed:** the first 1,500 run failed with
  `HashTable is full`. The engine's `HashTable` had a fixed 101 slots and no
  rehashing, and importing just 8 extra students onto the seed reproduced it.
  Because saving is one transaction, the failed run wrote nothing. I fixed it in
  PR #22 (`6ffdfe0`): `insert()` rehashes into about 2 × capacity once the load
  factor would pass 0.7. Value pointers are *moved*, so references from `find()`
  stay valid. Regression tests cover 1,000-key growth and a 1,500-student engine run.
- **Bug fixed while integrating PR #22:** a half-applied change in
  `PostgresConnection.cpp` used `!envHost` on a `std::string`, which doesn't
  compile. That PR also had a duplicate `getId()` and a deleted test file
  that CMake still referenced. All three are fixed and explained on the PR.
- **Two redundant indexes** found by profiling (`docs/PERFORMANCE.md`). Dropping
  them is recommended but not applied, pending team agreement.
- **Not mine, still pending:** the CLI screens that call `AllocationService` and
  `ReportRepository` (Member 4), the AVL waitlist ordering (Member 2), and
  `detectBlockingPairs()` returning a count (Member 1).

---

## 6. Likely evaluator questions (Member 3, Phase 3)

- **How do you stop two coordinators running an allocation at the same time?**
  A PostgreSQL advisory lock held by an RAII `RunLock`. The second run's
  `pg_try_advisory_lock` returns false and it throws. Because the unlock is in
  the destructor, even a crashed run releases the lock.

- **What happens if the program crashes while saving results?**
  Nothing is half-saved. Delete, insert, status update and seat recount all run
  in one transaction, and PostgreSQL rolls back anything that wasn't committed.

- **Why can't a coordinator just run the same round again?**
  SRS #7: results must never be silently overwritten. A second run of an
  allocated round throws `AlreadyAllottedException` unless `allowRerun` is
  passed explicitly.

- **Why did you replace `std::sort`? It's faster to write.**
  Two reasons. Sorting is the DS concept this module is graded on, so it should be
  our code. And `std::sort` isn't stable, which meant "keep the first duplicate"
  was never actually guaranteed. My merge sort is stable, and a test proves it.

- **How do you know the matching is really stable?**
  The test reads the saved allotments back and counts blocking pairs with an
  independent checker. To show the checker isn't fake, a second test plants a
  blocking pair on purpose and confirms it's detected.

- **Your RUN_LOG says NULL for blocking pairs. Isn't that missing data?**
  It's the honest value. The stability check isn't wired into the engine yet,
  and writing `0` would claim we proved something we didn't. The tests do verify
  stability; the audit log will record the real count once
  `detectBlockingPairs()` returns one.

- **What did profiling show?**
  All four hot queries use an index at 1,500 rows. The composite
  `(student_id, rank)` index means `findAll` gets preferences pre-sorted with no
  Sort step. It also showed that one lookup spends ~93% of its time in round trips,
  not query execution, which is the measured case for batching. And it found two
  duplicate indexes.

- **Did scale testing find anything?**
  Yes. The engine's hash table couldn't grow past 101 entries, so allocation
  failed above 101 students. We reproduced it, traced it to the missing rehash,
  and fixed it: grow at load factor 0.7 and re-hash every key, because the slot
  index depends on capacity. Now 1,500 students allocate in about 190 ms with 0
  blocking pairs. That is exactly what scale testing is for.

- **Why move the pointers during rehash instead of copying the values?**
  The engine keeps references returned by `find()`. Copying would put each value
  at a new address and leave those references dangling. Moving the `unique_ptr`
  keeps every value where it is; only the slot pointing at it changes. A test
  writes through an old reference after several rehashes to prove it.
