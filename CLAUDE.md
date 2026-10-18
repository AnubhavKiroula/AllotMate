# AllotMate

C++17, CMake, PostgreSQL (libpqxx). Gale-Shapley elective allocation. 3rd-sem PBL (DS + OOP).
Read first: docs/03_ARCHITECTURE.md, docs/05_DEVELOPMENT.md (Phase scope), CONTEXT.md.

## Code style
- Write simple code a 3rd-semester student can read and explain in a viva.
- Plain loops and clear names over clever templates, heavy abstractions or dense one-liners.
- Short functions, brief comments only where the "why" is not obvious.
- Use the data structures the docs name (priority queue, AVL, etc.); don't swap them.
- STL rule: `vector`, `string`, streams, smart pointers, `map` and `set` are allowed (OOP Unit 5). DS logic from the lectures must be hand-written: no `std::sort`, `<stack>`, `<queue>`, `std::priority_queue` or `std::list` in project logic, BST/AVL/linked list/graph built from our own nodes, and our own `HashTable` instead of `unordered_map`/`unordered_set`.

## Rules
- Touch only the files for the task. Teammates own other modules.
- Build and run existing tests before saying done; report what was and wasn't verified.
- Never commit .env or secrets.
