# Version Control Guidelines — AllotMate

> **For AI coding tools:** When asked to commit, always follow the Conventional Commits format in Section 2. Never suggest committing more than 1-2 logical file changes at once. Always suggest branching per Section 3 before writing new-feature code, and draft a PR description per Section 4 template when a branch's work is ready.

---

## 1. Why This Matters

- Clean commit history = easier debugging, easier code review, and a visible record of individual contribution (important since 4 members are being evaluated on real, distinguishable work).
- Frequent small commits > rare giant commits — easier to review, easier to revert if something breaks, and gives an honest picture of everyone's contribution graph.

---

## 2. Commit Message Format (Conventional Commits)

**Format:**
```
<type>: <short summary in present tense, under 50 chars>

<optional longer description — only if the change is non-trivial>
```

### Allowed types

| Type | Use for |
|---|---|
| `feat` | A new feature or capability |
| `fix` | Bug fix |
| `refactor` | Code restructuring with no behavior change |
| `chore` | Maintenance — deps, config, formatting, non-code housekeeping |
| `docs` | Documentation-only changes (including `/docs` files) |
| `test` | Adding or updating tests |
| `style` | Formatting/whitespace/naming, no logic change |
| `perf` | Performance improvement |
| `ci` | CI pipeline/workflow changes |
| `build` | Build system changes (CMakeLists.txt, Dockerfile, etc.) |
| `revert` | Reverting a previous commit |

### Examples

**Short, simple change:**
```
feat: add SparseMatrix insert and get methods
```

**Bug fix:**
```
fix: correct off-by-one error in AVL tree rebalance
```

**Bigger change — add a detailed body:**
```
feat: implement Gale-Shapley core matching loop

Adds student-proposing stable matching using the Stack-based
unmatched queue and HashTable indices for O(1) student/elective
lookup. Handles full rejection case (SRS edge case #5) by marking
student status as UNMATCHED rather than throwing.

Refs: 02_SRS.md edge case #5, #8
```

**Docs change:**
```
docs: add edge case table to SRS document
```

**Build/config:**
```
build: add libpqxx linkage to CMakeLists.txt
```

### Rules
- Always use **present tense** ("add", not "added"/"adds" — imperative mood, like giving a command).
- Keep the first line under ~50 characters; wrap body text at ~72 characters per line.
- Reference the relevant `/docs` file or edge-case number when a commit implements something specifically called out there (helps evaluators/reviewers trace requirements to code).
- **One logical change per commit.** Do not mix, e.g., a bug fix and a new feature in the same commit.

---

## 3. Commit Frequency

- **Commit after every 1–2 file changes**, not at the end of a whole feature. This means:
  - Wrote the header + basic class skeleton? Commit.
  - Implemented one method with a test? Commit.
  - Fixed one bug? Commit — separately from any feature work.
- Avoid single commits that touch 5+ unrelated files — split them.
- This isn't about padding commit count artificially — it's about making history genuinely reviewable and giving each member's contribution graph an honest, granular record.

---

## 4. Branching Workflow

**Before starting ANY new feature or fix:**

```bash
git checkout main
git pull origin main
git checkout -b <branch-name>
```

Never branch off a stale local `main` — always pull first, every single time, even if you branched an hour ago.

### Branch naming convention

```
<type>/<member-or-module>-<short-description>
```

| Example | Use case |
|---|---|
| `feature/m1-matching-engine` | New feature, Member 1, matching engine |
| `feature/m2-student-class` | New feature, Member 2, Student class |
| `fix/m3-connection-leak` | Bug fix, Member 3, DB connection issue |
| `refactor/m2-repository-interface` | Refactor, Member 2, repository interface |
| `chore/update-gitignore` | Housekeeping, no specific member needed |
| `docs/update-srs-edge-cases` | Documentation update |

- Use lowercase, hyphens (not underscores or spaces).
- Keep it short but specific enough that anyone reading the branch list understands what it's for without opening it.

---

## 5. Pull Request (PR) Guidelines

**When a branch's work is complete and tested locally:**

1. Push the branch:
   ```bash
   git push origin <branch-name>
   ```
2. Open a PR **into `dev`** (not directly into `main` — see `05_DEVELOPMENT.md` Git Workflow section).
3. **PR title** — brief, same style as a commit summary:
   ```
   feat: implement AVL tree waitlist with rebalancing
   ```
4. **PR description** — always in Markdown, using this template:

```markdown
## What this PR does
Brief 1-3 sentence summary of the change.

## Why
Which requirement/doc section this addresses (e.g., SRS FR-7, edge case #8).

## Changes
- Bullet list of concrete changes made
- Keep it scannable, not a wall of text

## How it was tested
- What you ran/tested manually or via unit tests
- Any known limitations or edge cases not yet handled

## Screenshots/output (if relevant)
Paste terminal output, test results, or CLI screenshots if useful for the reviewer.

## Checklist
- [ ] Compiles with zero warnings (`-Wall -Wextra`)
- [ ] Follows SOLID/DRY per 05_DEVELOPMENT.md
- [ ] Edge cases from 02_SRS.md Section 4 considered (list which, if applicable)
- [ ] No secrets/credentials committed
- [ ] At least one other member has reviewed
```

5. **Require at least one review** from another team member before merging into `dev` (per `05_DEVELOPMENT.md`).
6. Once approved, merge with **"Squash and merge"** if the branch has many small WIP commits, or a regular merge if the commit history is already clean and worth preserving as-is — team's call per PR, but default to squash to keep `dev`/`main` history readable.
7. Delete the branch after merging (keep the branch list clean).

---

## 6. Quick Reference Cheat Sheet

```bash
# Start new work
git checkout main
git pull origin main
git checkout -b feature/m1-priority-queue

# Work + commit often
git add <specific files>
git commit -m "feat: add PriorityQueue insert with heap sift-up"

# ... more small commits as you go ...

# Push and open PR
git push origin feature/m1-priority-queue
# → open PR into `dev` on GitHub, use template in Section 5
```

---

## 7. For AI Coding Tools Specifically

- When generating a commit message, always use the `<type>: <summary>` format from Section 2 — never a vague message like "update code" or "changes".
- When multiple unrelated changes exist in the working directory, suggest splitting them into separate commits rather than one combined commit.
- When a feature/fix is complete, proactively draft the PR title + Markdown description using the Section 5 template, ready to paste into GitHub.
- Always remind the user to `git pull origin main` before creating a new branch if it's been more than a session since the last pull.
