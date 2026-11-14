# Architecture Document — AllotMate

> **For AI coding tools:** Do not introduce new libraries, frameworks, or architectural patterns beyond what's listed here without flagging it to the team first. Stick to the module boundaries defined here.

---

## 1. Tech Stack

| Layer | Choice | Why |
|---|---|---|
| Core language | C++17 | Required for OOP syllabus; custom DS implementations (not STL shortcuts) for DS syllabus |
| Database | PostgreSQL | Relational, supports real scale (1500+ rows), strong indexing, free/open-source |
| DB connector | `libpqxx` | Official C++ client library for Postgres |
| Build system | CMake (generates Makefile) | Cross-platform, handles multi-file/multi-member builds and library linking cleanly |
| DB inspection tool | pgAdmin | GUI for humans to inspect/debug data — not used by the program itself |
| Version control | Git + GitHub (private repo) | Team collaboration, branch-per-module workflow |
| Containerization | Docker (optional, recommended) | See Section 5 |

## 2. APIs

**No external third-party APIs are used or required.** This is intentional — the project's value is the algorithm and architecture, not third-party integration.

If a web dashboard is built (optional, see UI/UX doc), it will consume a small internal REST layer exposed by the C++ core itself — **not** a third-party API.

## 3. Database

**PostgreSQL**, schema as defined in the ER diagram (see project design doc):
- `STUDENT`, `ELECTIVE`, `PREFERENCE`, `ALLOTMENT`, `WAITLIST`, `COORDINATOR`, `RUN_LOG`

Key decisions:
- Indexing on `roll_no`, `preference(student_id, rank)`, `allotment(elective_id)` for performance at scale.
- No ORM — direct SQL via `libpqxx`, wrapped in a `Repository` pattern (see class diagram) so the DB layer stays swappable and testable.
- Connection details (host, user, password) live in a `.env` file — **never committed to Git.**

## 4. Authentication

**Decision: minimal, not enterprise-grade — and this is intentional, say so if asked.**

This is an academic PBL, not a production system exposed to the public internet. Scope:
- **Coordinator login:** simple username/password check against a `coordinator` table, password stored as a **bcrypt hash** (never plaintext) — enough to demonstrate the concept correctly without over-engineering.
- **Student access:** no login required for v1 — students query their status by roll number (read-only). This can be extended to a simple token/session system as a stretch goal, but is explicitly **not required** for core evaluation.
- **No OAuth, no JWT, no third-party auth provider** — would be disproportionate engineering effort for the actual grading criteria (DS + OOP depth), and adds risk/complexity with little payoff for this scope.

If asked "why not a real auth system": *"Our evaluation criteria is algorithmic and architectural depth in DS/OOP — we scoped authentication minimally and deliberately, rather than spending limited time on infrastructure that isn't what we're being graded on."*

## 5. Docker — Yes, Recommended

**Use Docker for:**
- **PostgreSQL** — run Postgres in a container instead of requiring every team member to install/configure it locally. Eliminates "works on my machine" DB version mismatches.

**Do NOT containerize the C++ core itself** for this project — it adds build complexity (linking `libpqxx` inside a container, volume-mounting source, etc.) that isn't worth it for a college PBL demoed live on a laptop. Compile and run the C++ core natively on the demo machine.

**Suggested `docker-compose.yml` scope:**
```yaml
services:
  postgres:
    image: postgres:16
    environment:
      POSTGRES_DB: allotmate
      POSTGRES_USER: ${DB_USER}
      POSTGRES_PASSWORD: ${DB_PASSWORD}
    ports:
      - "5432:5432"
    volumes:
      - pgdata:/var/lib/postgresql/data

volumes:
  pgdata:
```

This means: any team member runs `docker compose up -d`, and Postgres is ready — no manual per-machine setup. The C++ core connects to `localhost:5432` as if Postgres were installed natively.

## 6. Module Boundaries (strict)

```
/src            → C++ core (matching engine, domain models, repository layer)
/sql            → Postgres schema + migrations
/INSTRUCTIONS   → this folder
```
