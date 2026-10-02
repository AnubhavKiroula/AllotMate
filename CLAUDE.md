# AllotMate

C++17, CMake, PostgreSQL (libpqxx). Gale-Shapley elective allocation. 3rd-sem PBL (DS + OOP).
Read first: docs/03_ARCHITECTURE.md, docs/05_DEVELOPMENT.md (Phase scope), CONTEXT.md.

## Code style
- Write simple code a 3rd-semester student can read and explain in a viva.
- Plain loops and clear names over clever templates, heavy abstractions or dense one-liners.
- Short functions, brief comments only where the "why" is not obvious.
- Use the data structures the docs name (priority queue, AVL, etc.); don't swap them.

## Rules
- Touch only the files for the task. Teammates own other modules.
- Build and run existing tests before saying done; report what was and wasn't verified.
- Never commit .env or secrets.
