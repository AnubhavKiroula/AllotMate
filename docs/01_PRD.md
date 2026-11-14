# Product Requirements Document (PRD) — AllotMate

> **For AI coding tools:** Always read this file first for *why* this project exists before generating or modifying any code. Do not suggest features or scope not aligned with this document without flagging it explicitly.

---

## 1. Purpose

AllotMate exists to replace manual and first-come-first-serve (FCFS) elective allocation with a **provably fair, algorithmic allocation system**, built as a combined Data Structures (C) + Object-Oriented Programming (C++) academic project, with real institutional applicability.

## 2. Problem Statement

Every semester, ~1500 students rank electives by preference against limited seats. Current resolution methods are:
- **Manual allocation** — slow, inconsistent, prone to human bias.
- **FCFS** — rewards speed/luck over fairness, provides no guarantee that the final allocation is optimal or stable for both sides.

There is no current system that guarantees mathematically that no student-elective pair would both prefer to swap out of their assigned allocation.

## 3. What We're Building

A **stable-matching allocation engine** (Gale-Shapley algorithm, student-proposing variant) that:
- Accepts ranked student preferences and elective seat capacities.
- Produces a final allocation that is **provably stable** (no blocking pairs).
- Persists all data in PostgreSQL for real semester-to-semester use.
- Provides a fairness verification report after every run.

## 4. Who This Is For

| User | Need |
|---|---|
| **Academic Coordinator / Exam Cell** | Run allocation each semester, set elective capacities, review fairness reports, resolve edge cases |
| **Student** | Submit ranked preferences, view final allotment/waitlist status |
| **Evaluators (DS + OOP)** | Assess implementation of syllabus concepts within a single coherent codebase |

## 5. Goals

- Solve the allocation problem correctly and provably.
- Demonstrate deep, real (non-STL-shortcut) implementation of DS concepts from the syllabus.
- Demonstrate clean OOP architecture (SOLID, inheritance, polymorphism, exceptions) in C++.
- Be usable at real scale (1500 students) — not a toy demo.

## 6. Non-Goals (explicitly out of scope for the core)

- Building a full production-grade multi-tenant SaaS system.
- Handling authentication/security to enterprise standard (see Architecture doc for the minimal approach we're taking and why).
- Mobile app or complex web frontend — CLI is the core interface; web/dashboard is optional stretch (see UI/UX doc).

## 7. Success Criteria

- Allocation runs end-to-end on seeded data at 1500-student scale within acceptable time.
- Fairness checker reports **0 blocking pairs** on every successful run.
- Both DS and OOP evaluators can independently identify their syllabus concepts clearly represented in the codebase.
- Project is demoable live without crashing, with a clear before/after (FCFS vs stable matching) comparison.

## 8. Reference Diagrams

For the UML class diagram, ER diagram, SOLID principle mapping, and the full pitch script, see:
**`00_PROJECT-OVERVIEW-AND-DIAGRAMS.md`**
---

**Any team member or AI tool proposing a new feature must check it against Sections 5 and 6 before implementing it.**
