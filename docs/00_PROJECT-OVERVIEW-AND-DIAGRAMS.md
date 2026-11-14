# AllotMate — Fair Elective Allocation Engine
### PBL Design Document (Data Structures in C + OOP in C++)

---

## 1. Overview

**Problem:** 1500 students rank electives by preference; seats are limited. Manual/FCFS allocation is unfair and unauditable.

**Solution:** A Gale–Shapley stable-matching engine (student-proposing) with capacity-aware colleges, backed by PostgreSQL, exposed via a thin service layer, written in modular C++ using classic data structures for every subsystem instead of generic STL shortcuts where the syllabus demands a real implementation.

**Tech stack**
- **Language:** C++17 (custom DS classes wrapping/replacing STL where required for evaluation)
- **DB:** PostgreSQL via `libpqxx`
- **Build:** CMake + Makefile
- **Interface:** CLI admin tool first → optional lightweight web dashboard (Phase 4, stretch)
- **Version control:** Git, feature-branch workflow, 1 branch per module owner

---

## 2. Module → Owner → Syllabus Mapping

| Module | Owner | Core DS used | Core OOP used |
|---|---|---|---|
| Matching Engine | Member 4 | Priority Queue, Stack (backtracking), Sparse Matrix | Strategy pattern, exceptions |
| Domain Models | Member 2 | — | Inheritance, polymorphism, operator overloading |
| Waitlist & Reporting | Member 2/4 | AVL Tree, Threaded BST, Hashing | Templates, iterators |
| Persistence Layer | Member 1 | Sorting (bulk import) | RAII, Repository pattern |
| Integration/Testing/Docs | Member 3 | — | Unit tests, benchmarks |

---

## 3. UML Class Diagram

```mermaid
classDiagram
    class Person {
        <<abstract>>
        -int id
        -string name
        +virtual void displayProfile()* 
        +virtual ~Person()
    }

    class Student {
        -float cgpa
        -vector~int~ preferenceList
        -Status status
        +addPreference(int electiveId)
        +displayProfile() override
        +operator<(Student other) bool
        +operator==(Student other) bool
    }

    class Coordinator {
        -string department
        +approveAllocation(Elective e)
        +displayProfile() override
    }

    class Elective {
        -int id
        -string name
        -int capacity
        -PriorityQueue~Student~ waitlist
        +hasSeat() bool
        +addToWaitlist(Student s)
        +operator<(Elective other) bool
    }

    class SparseMatrix {
        -vector~Triple~ preferenceEntries
        +insert(row, col, rank)
        +get(row, col) int
    }

    class HashTable~K,V~ {
        -vector~Bucket~ table
        +insert(K key, V val)
        +find(K key) V
        +remove(K key)
    }

    class AVLTree~T~ {
        -Node~T~* root
        +insert(T val)
        +rebalance(Node~T~* node)
        +inorderTraverse()
    }

    class ThreadedBST~T~ {
        -ThreadedNode~T~* root
        +insert(T val)
        +inorderTraverse() : no-stack
    }

    class MatchingEngine {
        -Stack~Student~ unmatchedStack
        -SparseMatrix prefMatrix
        -HashTable~int,Student~ studentIndex
        -HashTable~int,Elective~ electiveIndex
        +runGaleShapley()
        +detectBlockingPairs() vector
        +generateFairnessReport()
    }

    class AllocationException {
        <<exception>>
        +SeatOverflowException
        +InvalidPreferenceException
        +AlreadyAllottedException
    }

    class Repository~T~ {
        <<interface>>
        +save(T entity)
        +findById(int id) T
        +findAll() vector~T~
    }

    class PostgresStudentRepo {
        -pqxx::connection conn
        +save(Student s) override
        +findById(int id) Student override
        +findAll() vector~Student~ override
    }

    Person <|-- Student
    Person <|-- Coordinator
    MatchingEngine --> SparseMatrix
    MatchingEngine --> HashTable
    MatchingEngine --> AllocationException : throws
    Elective --> "1" AVLTree : waitlist backup
    MatchingEngine --> ThreadedBST : report generation
    Repository <|.. PostgresStudentRepo
    MatchingEngine --> Repository : uses (DIP)
    Student "many" --> "many" Elective : preferences
```

**SOLID touchpoints already baked in:**
- **S**: `MatchingEngine` only matches; `Repository` only persists; `Student`/`Elective` only hold state.
- **O**: New allocation strategies (e.g. merit-weighted) extend via a `Strategy` interface, no engine edits.
- **L**: `Student`/`Coordinator` are substitutable wherever `Person*` is expected.
- **I**: Separate `Repository<Student>` vs `Repository<Elective>`, no fat interfaces.
- **D**: `MatchingEngine` depends on `Repository` abstraction, not `PostgresStudentRepo` directly — swap DB layer without touching logic.

---

## 4. ER Diagram (PostgreSQL)

```mermaid
erDiagram
    STUDENT {
        int student_id PK
        varchar name
        varchar roll_no UK
        float cgpa
        varchar branch
        varchar status
    }

    ELECTIVE {
        int elective_id PK
        varchar name
        int capacity
        int seats_filled
        varchar semester
    }

    PREFERENCE {
        int pref_id PK
        int student_id FK
        int elective_id FK
        int rank
    }

    ALLOTMENT {
        int allotment_id PK
        int student_id FK
        int elective_id FK
        timestamp allotted_on
        varchar round
    }

    WAITLIST {
        int waitlist_id PK
        int student_id FK
        int elective_id FK
        int position
    }

    COORDINATOR {
        int coordinator_id PK
        varchar name
        varchar department
    }

    RUN_LOG {
        int run_id PK
        int coordinator_id FK
        timestamp run_at
        int blocking_pairs_found
    }

    STUDENT ||--o{ PREFERENCE : ranks
    ELECTIVE ||--o{ PREFERENCE : "ranked in"
    STUDENT ||--o| ALLOTMENT : "gets one"
    ELECTIVE ||--o{ ALLOTMENT : fills
    STUDENT ||--o{ WAITLIST : "waits on"
    ELECTIVE ||--o{ WAITLIST : maintains
    COORDINATOR ||--o{ RUN_LOG : triggers
```

**Indexing plan:** B-tree index on `student.roll_no`, composite index on `preference(student_id, rank)`, index on `allotment(elective_id)` for capacity checks at scale.

---

## 5. Phased Roadmap

| Phase | Deliverable | Weeks |
|---|---|---|
| **1. Design** | This doc finalized, Git repo + branch structure, CMake skeleton compiling empty modules | 1 |
| **2. Boilerplate** | `Person`/`Student`/`Coordinator`/`Elective` classes, empty `MatchingEngine`, DB schema created + migrations, dummy data seeded (100 → 1500 records) | 2 |
| **3. Modular build** | Each owner implements their DS class independently against agreed interfaces; unit tests per module | 3–5 |
| **4. Integration** | Wire `MatchingEngine` to `Repository`, run Gale-Shapley end-to-end on seeded data, fix mismatches | 6 |
| **5. Scale test + fairness report** | Load-test at 1500 students, benchmark runtime, blocking-pair detection report | 7 |
| **6. Polish + docs + viva prep** | CLI/dashboard, README, architecture writeup, rehearse both evaluator narratives | 8 |

---

## 6. Why This Project Is Competitive Across the Whole 2nd Year (Not Just DS/OOP)

Most peer projects will be CRUD apps with a database, or shallow pitches with no algorithmic depth. AllotMate differs on three axes:

1. **Provable correctness** — Gale-Shapley guarantees a *stable* matching (no blocking pairs). You can mathematically demonstrate this in the demo, not just claim it works.
2. **Real institutional utility** — this solves an actual problem the college has every semester, not a toy scenario.
3. **Full-stack engineering discipline** — SOLID, clean module boundaries, and DB-backed persistence at real scale (1500 students).

**Positioning line for judges:** *"Most teams show you code that runs. We're showing you an allocation system that is mathematically proven fair, and can be handed to the exam/academic cell tomorrow."*

---

## 7. Team Roles (Updated)

| Member | Role | Owns |
|---|---|---|
| **1 — DS/Algorithm Lead** | Matching engine: sparse matrix, hashing, priority queue, stack backtracking, threaded BST reporting | Presents to **DS evaluator** |
| **2 — OOP Architect** | Class hierarchy, inheritance, operator overloading, exceptions, STL, SOLID structure | Presents to **OOP evaluator** |
| **3 — Backend/DB Engineer** | Postgres schema, libpqxx integration, bulk import at 1500-record scale, indexing | Supports both |
| **4 — Integration, Testing & Docs** | End-to-end testing, benchmarking, demo build, documentation | Ties the whole story together for the panel |

*(If you're 5–6 members: split Member 3 into Testing+Docs and a dedicated Dashboard owner.)*

---

## 8. The Pitch (For Tomorrow's Approval)

**Duration target: ~3 minutes talk + have this doc ready if they want detail.**

> **Opening (hook — 15 sec)**
> "Every semester, 1500 students rank electives, and seats are limited — right now that's resolved manually or first-come-first-serve, which we can show is provably unfair. We're building a system that fixes that, permanently, for this college."

> **The core idea (30 sec)**
> "We're implementing Gale-Shapley stable matching — the same algorithmic idea behind national medical residency allocation — adapted for student-elective allocation with seat capacity. It doesn't just assign fast, it guarantees mathematically that there's no pair of a student and elective who'd both prefer to swap out of their current assignment. That's not a claim, it's provable, and we'll show the proof in our demo."

> **Why it's the right project for THIS syllabus (30 sec)**
> "It's a genuine combined DS + OOP project, not two projects stitched together. Every core data structure from our DS-in-C syllabus has a real job here: sparse matrices for the preference data, priority queues and stack-based backtracking for the matching itself, AVL trees and threaded binary trees for waitlists and reporting, hashing for instant lookups at 1500-student scale. And the entire system is architected in C++ following SOLID principles — inheritance and polymorphism for student/coordinator roles, operator overloading for comparisons, custom exception handling, and a clean repository pattern over a real PostgreSQL backend."

> **Scale & realism (20 sec)**
> "We're not demoing this on 20 fake students — we're building and load-testing it at real scale, 1500 records, so we can actually hand this to the exam or academic cell as a working tool, not just a submission."

> **Close / ask (15 sec)**
> "This is real, usable, provably fair, and scoped so every team member has clear, independent ownership. We'd like approval to start this week."

**If asked "why is this hard / novel":** *"Stable matching isn't in our syllabus at all — we chose it because FCFS and manual allocation are provably unfair, and we wanted to solve the actual problem, not just check syllabus boxes."*

**If asked "why C++ when DS is taught in C":** *"Our OOP syllabus is in C++, and since this is a joint evaluation, we implemented every data structure natively in C++ ourselves rather than leaning on STL shortcuts, so both subjects are genuinely represented in the same codebase."*

---

## 9. Next artifacts to build (in order)
1. Repo structure + CMakeLists.txt skeleton
2. Header-only interface definitions (`Person.h`, `Repository.h`, `AllocationException.h`) so all 4 members can code in parallel without blocking each other
3. Postgres schema `.sql` migration file

Say the word and I'll generate the actual boilerplate repo structure next.
