# AllotMate — Phase 2 Evaluation Guide (Backend & Database)

> **Scope of this document:** Phase 2 is worth 30 marks (mid-semester). This
> guide is written primarily around **Member 3 (Anubhav Kiroula — Backend &
> Database Lead)**, because that is the module that fully landed in Phase 2. It
> explains every file I added, what each function does, the concept behind it,
> and the questions an evaluator is likely to ask. The other members' Phase 2
> work (custom data structures, AVL/Threaded trees, CLI actions, ML scaffold)
> is tracked in `README.md` and will get its own write-ups as it merges.

---

## 1. Where Phase 2 fits

Phase 1 gave us the skeleton: the schema, the Docker database, the RAII
connection wrapper, the domain classes, and a CLI that could read the database.
What it did **not** have was a clean way for the rest of the engine to *talk* to
the database in terms of objects instead of raw SQL strings.

That is the gap Phase 2 closes on the backend side. My goal was:

1. Give the matching engine a way to load and store `Student` and `Elective`
   objects without knowing any SQL — the **Repository pattern**.
2. Let a coordinator import a real file of students (hundreds to a thousand-plus
   rows) safely, rejecting bad or duplicate rows instead of trusting the file.
3. Keep an audit trail of every matching run in `RUN_LOG`.
4. Stop paying the cost of opening a fresh database connection for every single
   query — reuse connections.
5. Prove all of the above with tests that don't endanger our seed data.

Everything below maps back to one of those five points.

---

## 2. The shape of the module (how the files connect)

```
                 CSV file on disk
                        |
                        v
                 CsvImporter  ──►  vector<StudentRecord>   (pure data, no DB)
                        |                     |
                        |                     v
                        |          PostgresStudentRepo::bulkInsert
                        |                     |
   ConnectionPool ──► PostgresConnection ◄────┘   (RAII, owns one libpqxx socket)
        (reuse)             ▲   ▲   ▲
                            |   |   |
        PostgresStudentRepo ┘   |   └ RunLogRepository
                                |
                   PostgresElectiveRepo
```

Two deliberate design lines run through this:

- **`CsvImporter` never touches the database.** Parsing and validation are pure
  logic, so they can be unit-tested on any machine with no PostgreSQL installed.
  Persistence is a separate step in the repository. This is separation of
  concerns in practice, and it is why half of our tests run anywhere.
- **The repositories do not own a connection; they borrow one** (`PostgresConnection&`).
  That keeps connection lifetime in one place and makes connection reuse possible.

---

## 3. Member 3 — files, functions, and the concept behind each

### 3.1 `include/StudentRecord.h`

A plain struct: `rollNo`, `name`, `cgpa`, `branch`. Nothing clever. The point of
it is what it *doesn't* have — no `libpqxx` types, no database logic. It is the
value that travels out of the CSV importer and into the bulk insert. Keeping a
"dumb" data holder here is what lets the parser be tested without a database.

> **Concept:** a boundary/transfer object. The messy outside world (a CSV file)
> is turned into a clean, validated in-memory value before it ever reaches SQL.

### 3.2 `include/CsvImporter.h` + `src/CsvImporter.cpp`

This is the ingestion front door. Expected columns: `roll_no, name, cgpa,
branch`. A header row is detected and skipped.

Returned types:

- `RowError { lineNumber, rawLine, reason }` — one rejected row, kept so the
  coordinator sees *which* line failed and *why* rather than a silent drop.
- `ImportResult { accepted, errors, duplicateCount, totalRows }` — the whole
  outcome of a parse.

Functions:

- **`parseFile(path)`** — opens the file; if it can't be opened it throws
  `std::runtime_error` (fail loud, don't return an empty result that looks like
  "0 students, no problem"). Delegates to `parseStream`.
- **`parseStream(istream&)`** — the real work. Reads line by line, skips blanks
  and the header, validates each row, and collects accepted records and errors.
  After reading, it **sorts the accepted rows by roll number and then removes
  duplicates in a single linear pass** over the now-adjacent rows.
- **`parseLine(line, out, reason)`** — splits one line on commas, checks there
  are exactly four fields, rejects empty roll/name/branch, and parses the CGPA
  with `std::stod` while checking the *entire* field was numeric (so `"8.5abc"`
  is rejected, not silently read as `8.5`) and that it falls in the 0–10 range.
- **`trim(s)`** — strips surrounding whitespace.

> **Concepts:** input validation at the boundary; **sorting as a tool** — an
> `O(n log n)` sort turns duplicate detection into one `O(n)` scan of neighbours,
> which is the "Sorting (bulk import)" DS concept for this module. Notice we fail
> *per row*, not per file: one bad line doesn't throw away the other 1,499.

### 3.3 `include/PostgresConnection.h` + `src/PostgresConnection.cpp` (added `quoteLiteral`)

I extended my Phase 1 connection wrapper with one method:

- **`quoteLiteral(value)`** — returns a SQL-safe, single-quoted literal, e.g.
  `O'Brien` becomes `'O''Brien'`. It uses libpqxx's `connection::esc()` to double
  any embedded quotes, then wraps the result in quotes.

> **Concept:** SQL-injection safety. Student names come from a file we don't
> control. If I built SQL by gluing raw strings together, a name containing a
> quote could break out of the string and corrupt the statement. Every string
> value my repositories put into SQL goes through `quoteLiteral` first. Numbers
> are converted with `std::to_string` / fixed formatting, so they can't carry
> injection either.

### 3.4 `include/PostgresStudentRepo.h` + `src/PostgresStudentRepo.cpp`

This is the concrete `Repository<Student>`. It takes a `PostgresConnection&` in
its constructor and implements the three interface methods plus a bulk loader.

- **`findById(id)`** — `SELECT`s the student row; if there is no such row it
  throws `DatabaseException` (never returns a half-built object or a `-1`).
  Builds a `Student` and attaches its preferences.
- **`findAll()`** — loads every student ordered by id, then loads *all*
  preferences in **one** extra query and distributes them to the right students
  using an `unordered_map<id, index>`. The reason for the map is performance: the
  naive version would run one preference query per student (the classic "N+1
  query" problem); at 1,500 students that's 1,500 round trips. My version is
  always **two** round trips regardless of scale.
- **`save(student)`** — tries an `UPDATE` first (the common case: the engine
  changed a student's status to `ALLOTTED` and we persist it). If no row was
  updated, it `INSERT`s. Because the `Student` model has no roll number or branch
  (those belong to the OOP module's class, which I don't modify), the insert path
  fills those required columns with deterministic placeholders. Real students
  with real roll numbers come in through `bulkInsert`, not here — I was explicit
  about that split rather than quietly changing a teammate's class.
- **`bulkInsert(records)`** — the high-performance path. It groups records into
  chunks of 500 and emits **one multi-row `INSERT` per chunk**, so a thousand
  students become two statements, not a thousand. Each statement ends with
  `ON CONFLICT (roll_no) DO NOTHING`, so a roll number that already exists (for
  example, a row already seeded) is skipped instead of aborting the whole batch.
  It returns the count of rows actually inserted.
- **`mapRow(...)`** (private) — turns one row + its preferences into a `Student`.

Two small private helpers in the `.cpp` convert between the DB status text
(`UNASSIGNED` / `ALLOTTED`) and the `Status` enum, and format a CGPA to two
decimals so it fits the `NUMERIC(4,2)` column cleanly.

> **Concepts:** Repository pattern and the **Dependency Inversion Principle** —
> the matching engine will depend on `Repository<Student>`, not on PostgreSQL, so
> the database could be swapped for an in-memory fake in a test without touching
> the engine. Also: batch inserts, the N+1 query problem, and upsert semantics
> via `ON CONFLICT`.

### 3.5 `include/PostgresElectiveRepo.h` + `src/PostgresElectiveRepo.cpp`

The same pattern for `Elective` — `save`, `findById`, `findAll`.

The one interesting detail is `mapRow`: the `Elective` class exposes
`allocateSeat()` but no direct "set seats filled" setter. So when I load a row
whose `seats_filled` is, say, 7, I call `allocateSeat()` seven times to rebuild
the object's state through its real API. The schema guarantees
`0 <= seats_filled <= capacity`, so this replay can never trip the overflow
exception. `save()` is an update-or-insert just like the student repo, and if a
coordinator tries to set a capacity *below* the seats already filled, the
database `CHECK` constraint rejects it and that surfaces as a `DatabaseException`
— exactly the "fail loud" behaviour we want.

> **Concept:** reconstructing object state only through the object's own public
> methods (respecting encapsulation) instead of reaching past it.

### 3.6 `include/RunLogRepository.h` + `src/RunLogRepository.cpp`

`RUN_LOG` records an *event*, not an entity, so I deliberately did **not** force
it into `Repository<T>` (which is CRUD-shaped). Instead:

- **`logRun(coordinatorId, blockingPairsFound, summary)`** — appends a row and
  returns the generated `run_id` using `INSERT ... RETURNING run_id`. A
  coordinator id `<= 0` is stored as SQL `NULL` (a run not tied to anyone),
  which is why the column is nullable.
- **`recentRuns(limit)`** — reads the newest runs for the audit screen, handling
  the nullable `coordinator_id` and `outcome_summary` columns via `is_null()`.

`RunLogEntry` is the small struct these reads return.

> **Concept:** choosing the right interface for the data. Not everything is an
> entity with `findById`; an append-only audit log wants an append + tail-read
> API. Also demonstrates `RETURNING`, PostgreSQL's way to get a generated key
> back in the same statement.

### 3.7 `include/ConnectionPool.h` + `src/ConnectionPool.cpp`

A small pool that hands out reusable connections instead of reconnecting per
query.

- **`ConnectionPool(config, maxIdle)`** — remembers how to build a connection
  and how many idle ones to keep.
- **`acquire()`** — returns a `PooledConnection` lease wrapping either an idle
  connection or, if none are free, a freshly opened one.
- **`release(connection)`** — called automatically when a lease is destroyed;
  keeps the connection for reuse if it's still open and there's room, otherwise
  lets it close.
- **`PooledConnection`** — the lease. It is **move-only** and, in its destructor,
  returns its connection to the pool. This is the same RAII idea as the
  connection wrapper itself: you never call "release" by hand, it happens when
  the lease goes out of scope. `operator->` / `operator*` let you use it like a
  pointer to the connection.

> **Concepts:** RAII applied a second time (a lease that auto-returns), and
> **move semantics** — a lease owns a unique resource, so it can be moved but not
> copied; the move constructor nulls out the moved-from lease so the connection
> isn't returned twice. For a single-threaded CLI this mainly avoids repeated TCP
> connect handshakes during a burst of queries; the design also leaves room to
> add locking later if we ever go multi-threaded.

### 3.8 Tests — `tests/`

- **`test_csv_importer.cpp`** (no database) — checks that valid rows are accepted
  and sorted, that duplicate roll numbers are dropped and counted, that malformed
  rows are reported with their original line numbers (not crashing), and that an
  `Elective` refuses to allocate past its capacity (the **capacity property**).
  Because it needs no database, it runs on any machine and in CI.
- **`test_db_layer.cpp`** (needs PostgreSQL, else skips) — if it can't reach a
  database it prints `[SKIP]` and exits 0 so `ctest` stays green. When a database
  *is* present it tests the repositories, the run log, and the pool, and it opens
  with a **transactional rollback** test: it inserts a row inside a `pqxx::work`
  that is never committed, confirms the row is visible inside the transaction,
  lets the transaction fall out of scope, and then confirms the row is gone. That
  is how we test writes without ever damaging the seed data. Every committed
  write in the rest of the file uses a sentinel roll number (`ITEST…`) and is
  deleted again at the end.

> **Concept:** testing at two levels — fast pure-logic tests that always run, and
> integration tests that are honest about needing a real database and protect
> shared data with transactions.

### 3.9 Build & CI changes

- **`CMakeLists.txt`** — I pulled the non-`main` sources into a single static
  library `allotmate_core`, which both the `AllotMate` executable and the two test
  executables link against. This avoids compiling the same files twice and keeps
  the warning policy (`-Wall -Wextra -Werror`) in one place. Tests are registered
  with CTest.
- **`.github/workflows/ci.yml`** — I added a **PostgreSQL 16 service container**
  to the CI job, load the schema and seed into it, and run `ctest`. This is the
  important one: it means the database-layer integration tests actually execute
  on every pull request, not just the pure-logic ones.

---

## 4. Live demo flow for the Phase 2 backend

```bash
docker compose up -d                       # start PostgreSQL + seed
cmake -B build -S . && cmake --build build  # build app + tests
ctest --test-dir build --output-on-failure  # run both test suites
```

- Point at the test output: `csv_importer` passing shows validation, dedup and
  the capacity property; `db_layer` passing shows the repositories, run-log and
  pool working against the live database, with the rollback test proving seed
  data is safe.
- Open `data/students_sample.csv` to show the import format the `CsvImporter`
  consumes.
- On GitHub, open the latest CI run to show the same tests passing against a
  PostgreSQL service container with zero compiler warnings.

---

## 5. Likely evaluator questions (Member 3)

- **Why a Repository instead of calling SQL from the engine?**
  So the matching engine depends on an interface (`Repository<Student>`), not on
  PostgreSQL. We can swap the database or drop in a fake for testing without
  changing the engine. That's the Dependency Inversion Principle.

- **How do you stop a bad CSV from corrupting the database?**
  Two layers. First the `CsvImporter` validates every row (field count, empty
  fields, numeric and in-range CGPA) and drops duplicate roll numbers before any
  SQL runs. Then the database itself is the backstop: the `UNIQUE(roll_no)`
  constraint plus `ON CONFLICT DO NOTHING` means even a duplicate that slips
  through is skipped rather than inserted twice.

- **Why `quoteLiteral`? Isn't this a college project?**
  It's one line and it removes a whole class of bug. Names come from an outside
  file, and a stray apostrophe (think "O'Brien") would break a naively built SQL
  string. Escaping every string value is the correct habit, not extra effort.

- **Your bulk insert — why chunks of 500 and not one row at a time?**
  One row at a time is one network round trip per student. Chunked multi-row
  inserts turn a thousand students into two statements. The chunk cap just keeps
  any single statement a sensible size.

- **How do your tests avoid wrecking the seed data the other members rely on?**
  The write test runs inside a transaction that is never committed, so PostgreSQL
  rolls it back automatically. Anything that must be committed uses a sentinel
  roll number and is deleted afterward.

- **What's the N+1 query problem and where did it apply?**
  Loading each student's preferences with its own query would mean one query per
  student — N+1 queries total. In `findAll` I load all preferences in a single
  query and hand them out via a hash map, so the whole call is two queries
  regardless of how many students there are.

- **Why does `save()` use placeholders for roll number and branch?**
  The `Student` class (owned by the OOP module) doesn't carry those fields, and I
  don't modify another member's class. Real rows with real roll numbers come in
  through the CSV importer; `save()` is for persisting the columns a `Student`
  object actually has, such as status after a matching run.

---

## 6. Honest status — what is verified, and where

- **Pure-logic tests** (`test_csv_importer`): run and pass locally and in CI.
- **Repository SQL**: every statement the repositories generate was run against a
  real PostgreSQL 16 container and behaves correctly (including the capacity
  `CHECK` firing and apostrophe escaping).
- **Full `libpqxx` build + `test_db_layer`**: verified by CI, which builds on
  Ubuntu with `libpqxx-dev` and runs the tests against a PostgreSQL service.
  (A local Windows/MinGW toolchain can't link `libpqxx`, which is exactly why the
  CI gate exists.)

---

## 7. Other members — Phase 2 (in progress)

These are being built in parallel and will be documented here as they merge; I'm
not claiming them as done:

- **Member 1 (DS/Algorithm):** `SparseMatrix`, `HashTable`, `Stack`,
  `PriorityQueue`, and the Gale–Shapley loop.
- **Member 2 (OOP):** `AVLTree`/`ThreadedBST`, polymorphic profile rendering,
  model unit tests, rule-of-five for the tree structures.
- **Member 4 (Integration/ML):** full CLI actions, CTest scaffold expansion, and
  the isolated ML microservice scaffold.
