# Development Document — AllotMate

> **For AI coding tools:** Follow the phase order below strictly. Do not generate Phase 3+ code before Phase 1–2 interfaces exist and are agreed. Always write code that satisfies SOLID principles (see Section 4) and matches the class diagram in the project design doc — do not invent new classes or restructure existing ones without flagging it.

---

## 1. Development Principles (non-negotiable)

- **SOLID** — every class has one responsibility; new behavior extends via interfaces, not edits to existing classes; depend on abstractions (`Repository`), not concrete DB code.
- **DRY** — no duplicated matching/validation logic between modules; shared logic goes in a common utility, not copy-pasted.
- **RAII** — all resource management (DB connections, file handles) uses constructors/destructors, no manual cleanup calls scattered around.
- **Fail loud, not silent** — every invalid state throws a custom exception (see SRS Section 4 edge cases); nothing fails silently or returns ambiguous null/-1 values.
- **Test as you build** — each module owner writes basic unit tests for their own class before integration, not after.

## 2. Phases

### Phase 1 — Design (Week 1)
- Finalize this doc set (PRD, SRS, Architecture, UI/UX, Development).
- Set up GitHub repo (private), branch protection on `main`, add all members as collaborators.
- Set up Docker Compose for Postgres + ML service stub.
- Agree on header-only interfaces: `Person.h`, `Repository.h`, `AllocationException.h`.

**Exit criteria:** repo compiles an empty skeleton; Postgres container runs; every member has pulled and built successfully once.

### Phase 2 — Boilerplate (Week 2)
- Implement domain models: `Person`, `Student`, `Coordinator`, `Elective` (no logic yet, just structure + constructors).
- Create Postgres schema via `.sql` migration matching the ER diagram.
- Seed dummy data (start with ~100 records, script to scale to 1500 later).
- Stub `MatchingEngine`, `Repository` interfaces — empty methods, just signatures.

**Exit criteria:** can insert/read a dummy student to/from Postgres via `libpqxx` from C++.

### Phase 3 — Modular Build (Weeks 3–5)
Each owner builds independently against agreed interfaces (parallel work, minimal blocking):

| Owner | Builds |
|---|---|
| Member 1 (DS/Algorithm) | `SparseMatrix`, `HashTable`, `Stack`-based backtracking, `PriorityQueue`, core Gale-Shapley loop in `MatchingEngine` |
| Member 2 (OOP) | Full `Student`/`Coordinator`/`Elective` behavior, operator overloads, exception classes, `AVLTree`/`ThreadedBST` for waitlist/reporting |
| Member 3 (Backend/DB) | `PostgresStudentRepo` and other repo implementations, bulk CSV import at scale, indexing, `.env` config handling |
| Member 4 (Integration/Testing/ML) | Unit test scaffolding, CLI skeleton (Section 2 of UI/UX doc), begin ML microservice stub in parallel (isolated, no dependency on core progress) |

**Exit criteria:** each module has passing unit tests in isolation; no module depends on another module's *implementation*, only its *interface*.

### Phase 4 — Integration (Week 6)
- Wire `MatchingEngine` to real `Repository` implementations.
- Run full Gale-Shapley end-to-end on seeded data (start at 100, then 500 records).
- Implement `detectBlockingPairs()` fairness verification (see project design doc — this is the "proof" step).
- Fix mismatches between modules (expect interface friction here — this is normal, budget time for it).

**Exit criteria:** one full run — import → set capacities → allocate → 0 blocking pairs reported — works without manual intervention.

### Phase 5 — Scale Test + Fairness Report (Week 7)
- Load test at full 1500-student scale; record and document runtime.
- Build the FCFS-vs-stable-matching comparison demo (see pitch doc) — run both on identical data, show blocking pairs found in FCFS vs 0 in AllotMate.
- If time permits: connect ML microservice's forecast output into `elective.capacity` and demo the collusion-flagging endpoint.

**Exit criteria:** system handles 1500-record run within acceptable demo time; comparison demo works reliably.

### Phase 6 — Polish, Docs, Viva Prep (Week 8)
- Finalize CLI UX (clear prompts, error messages match SRS edge cases).
- (Stretch, only if ahead of schedule) build minimal web dashboard per UI/UX doc Section 3.
- Write final README, architecture writeup for submission.
- Rehearse the live demo end-to-end at least twice as a full team, including the "what if it breaks" fallback (keep a pre-recorded run/output as backup).
- Rehearse both evaluator narratives (DS-focused walkthrough, OOP-focused walkthrough) separately.

**Exit criteria:** demo runs successfully twice in a row without intervention; every member can independently explain both the algorithm and their own module.

## 3. Git Workflow

- `main` — always demo-stable, protected, merge via PR only.
- `dev` — integration branch, where Phase 4+ work lands before `main`.
- `feature/<member>-<module>` — individual work, e.g. `feature/m1-matching-engine`.
- PR requires at least one other member's review before merging to `dev`.
- Commit messages: `[module] short description` (e.g., `[matching-engine] implement stack-based backtracking`).

## 4. Definition of Done (per module)

A module is "done," not just "written," when:
1. It compiles with zero warnings under `-Wall -Wextra`.
2. It has at least basic unit tests covering the edge cases relevant to it (see SRS Section 4).
3. It throws proper custom exceptions instead of crashing or returning silent failure values.
4. Another team member has reviewed and can explain what it does.
5. It's documented with a short comment block explaining its role (helps both evaluators and future AI-tool context).

## 5. For AI Coding Tools Specifically

- Always check `/INSTRUCTIONS/02_SRS.md` Section 4 (edge cases) before considering a function complete.
- Always check `/INSTRUCTIONS/03_ARCHITECTURE.md` module boundaries before suggesting cross-module calls (e.g., never suggest calling Python directly from C++).
- Prefer extending via interfaces over modifying existing class signatures — flag it explicitly if a signature change seems necessary.
- Never introduce a new external library/dependency without flagging it to the team first — check Section 1 of the Architecture doc for the agreed stack.

## 6. Reference Diagrams

For the UML class diagram, ER diagram, and phased roadmap visuals, see:
**`00_PROJECT-OVERVIEW-AND-DIAGRAMS.md`**