# System Load Test — 1,500 Students × 30 Electives (Member 4)

End-to-end load test of AllotMate at full scale, run **through the interactive CLI**
(the way a coordinator uses it), plus Member 3's `benchmark_db` tool for comparison
with [`PERFORMANCE.md`](PERFORMANCE.md).

Run on 2026-11-13 by Gaurang Nauriyal.

## Setup

| Item | Value |
|---|---|
| Dataset | `scripts/scale_seed_1500.sql`: 1,500 students · 30 electives (1,521 seats, capacities 23–80) · 7,500 preferences (5 each) |
| Machine | Intel Core i7-13620H, 15.7 GB RAM, Windows 11 Home |
| Build | MSYS2 UCRT64, g++ 16.2.0, libpqxx 7.10 (built from source, C++17), `-Wall -Wextra -Werror` |
| Database | PostgreSQL 16.15 in Docker Desktop (`docker-compose.yml`) |
| Round | `Round 1` (the CLI's `CURRENT_ROUND`) |

## Correctness at scale

| Check | Result |
|---|---|
| Matched / unmatched (CLI, first run) | **1,426 matched, 74 unmatched** of 1,500 |
| Same result on 4 confirmed re-runs and 3 `benchmark_db` runs | Yes, identical every time (deterministic) |
| Electives over capacity after the run | **0** |
| Allotment rows saved for `Round 1` | 1,426 (equals "matched") |
| `test_allocation` on the 1,500 dataset | **All 18 checks pass**, including "no blocking pairs in the saved result" |
| `RUN_LOG.blocking_pairs_found` | `NULL` for every run, as intended until `detectBlockingPairs()` is implemented |
| View Results, all electives | 30 elective groups, 1,500 student lines, totals 1,426 / 74 |

## Timing

CLI times are measured **outside** the program (wall clock for one whole CLI session
with scripted input), so they include starting the program and connecting to the
database. A session that only logs in and exits is measured as a baseline; the
allocation's own cost is roughly *session time minus baseline*.

| Measurement | Runs | Result |
|---|---|---|
| CLI baseline: log in + exit | 5 | 233 – 324 ms (median 258 ms) |
| CLI: first allocation of `Round 1` | 1 | 837 ms session → **≈ 580 ms** allocation |
| CLI: confirmed re-run (replaces old results) | 4 | 747 – 977 ms sessions → **≈ 490 – 720 ms** allocation |
| CLI: View Results, all electives (1,500 lines) | 1 | 336 ms session |
| CLI: View Results, one elective | 1 | 263 ms session |
| `benchmark_db --allocate`: full allocation run | 3 | 531.65 / 608.22 / 914.30 ms (median 608 ms) |
| `benchmark_db`: CSV parse + validate + merge sort (1,500 rows) | 3 | 21 – 28 ms |
| `benchmark_db`: `bulkInsert` (1,500 rows) | 3 | 184 – 223 ms |
| `benchmark_db`: one lookup by `roll_no` | 3 × 1,500 | 4.23 – 4.73 ms |

## Comparison with `PERFORMANCE.md`

Member 3 measured the same full allocation at **186 – 196 ms** (Ubuntu). On this
Windows machine it takes about **530 – 910 ms**, roughly 3–5× slower.

The matching code and the data are the same, so the difference comes from the
environment. The clearest sign is a single `roll_no` lookup: **4.2 – 4.7 ms here vs
0.72 – 0.74 ms** in `PERFORMANCE.md`. That document already showed ~93% of a lookup is
client↔server round-trip time, so slower round trips (likely Docker Desktop's network
path between Windows and its Linux VM) explain most of the gap. This is the likely
cause, not a measured one.

**Conclusion:** a full 1,500-student allocation finishes in **under 1 second** through
the CLI on a normal Windows laptop, which is fine for the live demo.

## Issue found: `test_db_layer` assumes the 100-student seed

On the 1,500-student dataset, `test_db_layer` reports 2 failed checks:

- `allotmentResults filters by elective`
- `waitlistFor returns the queue front first`

Both compare against roll numbers from `sql/02_seed_data.sql` (`2024CS001`, `2024CS004`),
but in the scale dataset student 1 is `2024SC0001`. The code under test is fine; the
test data assumption is not. Both checks pass on the normal 100-student seed.
Suggested fix for the test's owner (Member 3): read the expected roll numbers from the
`STUDENT` table by `student_id` instead of hard-coding them.

## How to reproduce

PowerShell, from the project folder, with Docker Desktop running. **The first command
replaces all data in the local database.**

```powershell
cmd /c "docker exec -i allotmate_postgres psql -U postgres -d allotmate < scripts\scale_seed_1500.sql"
.\build\tests\test_allocation.exe
.\build\benchmark_db.exe --allocate
.\build\AllotMate.exe
```

In the CLI: `1` (Coordinator) → `5` (Run Allocation) → `6` (View Results, `0` = all).

Restore the normal 100-student seed afterwards:

```powershell
cmd /c "docker exec -i allotmate_postgres psql -U postgres -d allotmate < sql\02_seed_data.sql"
```

(`cmd /c` with `<` passes the file through unchanged; piping it with `Get-Content` in
Windows PowerShell 5.1 can change its encoding.)
