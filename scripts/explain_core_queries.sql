-- Index profiling for AllotMate's hot queries (run on the 1,500 scale dataset).
--
--   docker exec -i allotmate_postgres psql -U postgres -d allotmate < scripts/explain_core_queries.sql
--
-- Everything runs inside one transaction that is ROLLED BACK, so the
-- synthetic ALLOTMENT rows used for query 4 never persist.

\echo '=== Indexes defined on the profiled tables ==='
SELECT tablename, indexname, indexdef
FROM pg_indexes
WHERE tablename IN ('student', 'preference', 'allotment')
ORDER BY tablename, indexname;

BEGIN;

\echo ''
\echo '=== 1. Student lookup by roll number (student portal / duplicate check) ==='
EXPLAIN (ANALYZE, BUFFERS, COSTS OFF)
SELECT student_id FROM STUDENT WHERE roll_no = '2024SC0750';

\echo ''
\echo '=== 2. One student''s preferences in rank order (PostgresStudentRepo::findById) ==='
EXPLAIN (ANALYZE, BUFFERS, COSTS OFF)
SELECT elective_id FROM PREFERENCE WHERE student_id = 750 ORDER BY rank ASC;

\echo ''
\echo '=== 3. All preferences ordered for the merge-join (PostgresStudentRepo::findAll) ==='
EXPLAIN (ANALYZE, BUFFERS, COSTS OFF)
SELECT student_id, elective_id FROM PREFERENCE ORDER BY student_id, rank ASC;

-- Synthetic allotments (each student's first choice) purely so query 4 has
-- 1,500 rows to plan against. Rolled back at the end.
INSERT INTO ALLOTMENT (student_id, elective_id, round)
SELECT student_id, elective_id, 'PROFILE' FROM PREFERENCE WHERE rank = 1;
ANALYZE ALLOTMENT;

\echo ''
\echo '=== 4. Students allotted to one elective (capacity checks / results screen) ==='
EXPLAIN (ANALYZE, BUFFERS, COSTS OFF)
SELECT student_id FROM ALLOTMENT WHERE elective_id = 5;

ROLLBACK;
