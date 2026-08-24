# AllotMate — Fair Elective & Lab-Batch Allocation Engine

[![CI Build Gate](https://github.com/AnubhavKiroula/AllotMate/actions/workflows/ci.yml/badge.svg)](https://github.com/AnubhavKiroula/AllotMate/actions/workflows/ci.yml)
![Language](https://img.shields.io/badge/Language-C%2B%2B17-blue.svg)
![Database](https://img.shields.io/badge/Database-PostgreSQL%2016-336791.svg)
![Container](https://img.shields.io/badge/Docker-Compose-2496ED.svg)
![License](https://img.shields.io/badge/License-MIT-green.svg)

> **Provably Fair Student-Elective Matching System**  
> Combined Data Structures (C/C++) + Object-Oriented Programming (C++) Academic Project (PBL) with real institutional applicability for 1,500+ students.

---

## 📌 Problem & Solution Overview

- **The Problem:** Every semester, 1,500+ students rank elective courses and lab batches against strict seat limits. Existing manual or First-Come-First-Serve (FCFS) allocations reward speed or luck, are vulnerable to human bias, and offer **no stability guarantee** (students and electives may form blocking pairs who both prefer to swap).
- **The Solution:** **AllotMate** adapts the **Gale–Shapley stable matching algorithm** (student-proposing variant) with seat capacities, custom data structures (Sparse Matrix, Priority Queue, AVL Trees, Threaded BST, Hash Tables), an RAII-compliant PostgreSQL persistence layer via `libpqxx`, and an optional decoupled ML forecasting microservice.

---

## 📖 Documentation Quick Links

Comprehensive project documentation is available in the [`/docs`](./docs) directory:
- [`00_PROJECT-OVERVIEW-AND-DIAGRAMS.md`](docs/00_PROJECT-OVERVIEW-AND-DIAGRAMS.md) — UML Class Diagram, ER Diagram, SOLID Mapping & Evaluation Pitch
- [`01_PRD.md`](docs/01_PRD.md) — Product Requirements Document
- [`02_SRS.md`](docs/02_SRS.md) — Software Requirements Specification & Edge Cases
- [`03_ARCHITECTURE.md`](docs/03_ARCHITECTURE.md) — System Architecture, DB & Containerization
- [`04_UIUX.md`](docs/04_UIUX.md) — CLI Workflow & Evaluation Screens
- [`05_DEVELOPMENT.md`](docs/05_DEVELOPMENT.md) — Development Phases, SOLID Rules & Definition of Done
- [`06_VERSION-CONTROL.md`](docs/06_VERSION-CONTROL.md) — Conventional Commits, Branching Strategy & PR Guidelines
- [`DB-SETUP.md`](docs/DB-SETUP.md) — Database Quickstart & pgAdmin Setup Guide