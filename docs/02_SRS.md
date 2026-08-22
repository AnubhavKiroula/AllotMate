# Software Requirements Specification (SRS) — AllotMate

> **For AI coding tools:** This defines exact scope. Do not implement features not listed here without confirming with the team first. When generating code, validate against the edge cases in Section 4 — most bugs in this project will come from unhandled edge cases, not core logic.

---

## 1. Functional Requirements

### 1.1 Student-facing
| ID | Feature | Description |
|---|---|---|
| FR-1 | Submit preferences | Student ranks N electives in order of preference |
| FR-2 | View allotment status | Student can query their final allotted elective or waitlist position |
| FR-3 | Preference validation | System rejects duplicate ranks, invalid elective IDs, or preference lists exceeding available electives |

### 1.2 Coordinator-facing
| ID | Feature | Description |
|---|---|---|
| FR-4 | Set elective capacity | Coordinator defines/updates seat count per elective before a run |
| FR-5 | Trigger allocation run | Coordinator initiates the Gale-Shapley matching process |
| FR-6 | View fairness report | System outputs blocking-pair count and details after each run |
| FR-7 | View waitlist | Coordinator can see ranked waitlist per elective (AVL-tree backed) |
| FR-8 | Re-run allocation | If capacities change post-run, coordinator can trigger a fresh run without data corruption |
| FR-9 (optional) | View ML demand forecast | Coordinator sees predicted demand per elective before setting capacity |
| FR-10 (optional) | View collusion flags | Coordinator sees flagged suspicious preference clusters for manual review |

### 1.3 System-level
| ID | Feature | Description |
|---|---|---|
| FR-11 | Bulk import | Import student/elective/preference data from CSV at 1500-record scale |
| FR-12 | Persistence | All allocation runs, results, and waitlists persist in PostgreSQL |
| FR-13 | Run logging | Every allocation run is logged with timestamp, coordinator, and outcome summary |

## 2. Non-Functional Requirements

| ID | Requirement |
|---|---|
| NFR-1 | System must process 1500 students × ~30 electives within a demo-acceptable time (target: under a few seconds for matching itself; DB I/O may add more) |
| NFR-2 | Core C++ engine must have zero runtime crashes on malformed/edge-case input — all failures must be caught and reported via custom exceptions |
| NFR-3 | Codebase must follow SOLID principles — verified during code review before each merge |
| NFR-4 | ML microservice (if built) must be fully decoupled — core engine must function correctly with the ML service offline |
| NFR-5 | All DB credentials must be in `.env`, never hardcoded or committed |

## 3. What Users Can Do — Summary Flow

**Student flow:** submit preferences → (wait for run) → view result (allotted / waitlisted).
**Coordinator flow:** import data → set capacities → (optional: check ML forecast) → run allocation → review fairness report → resolve waitlist/edge cases → finalize.

## 4. Edge Cases (must be explicitly handled, not assumed away)

| # | Edge case | Expected behavior |
|---|---|---|
| 1 | Student submits fewer preferences than required | Accept, but they may go entirely unmatched — must be reported clearly, not silently dropped |
| 2 | Student ranks the same elective twice | Reject with `InvalidPreferenceException` |
| 3 | Elective has zero capacity | No students matched to it; must not crash matching loop |
| 4 | All students prefer the same single elective | Only capacity-many matched; rest proceed to next preference or waitlist correctly |
| 5 | A student's entire preference list is exhausted (all rejected) | Student ends as formally "unmatched" — must be a defined state, not an error |
| 6 | Duplicate roll numbers in bulk import | Reject import row, log it, continue processing rest of file (don't abort entire import) |
| 7 | Elective capacity reduced after a run already allocated more students than the new capacity | Must be explicitly resolved by coordinator (re-run policy) — never silently overwrite |
| 8 | Tie in CGPA/rank when an elective must choose between students | Deterministic tie-break rule required (e.g., by roll number) — must not be random/undefined behavior |
| 9 | Empty database on first run (no historical data) | ML forecast module must degrade gracefully (return "insufficient data", not crash) |
| 10 | Concurrent allocation runs triggered by mistake | Must be prevented/locked at DB level — no double-processing |

## 5. Out of Scope for v1

- Real-time multi-user concurrent editing of preferences.
- Full authentication/RBAC system (see Architecture doc for the minimal approach).
- Mobile/responsive web client.

---

**Any AI-generated code must be checked against Section 4 edge cases before being considered complete.**
