-- AllotMate scale dataset: 1,500 students x 30 electives, 5 ranked preferences each.
--
-- WARNING: this REPLACES all data (same TRUNCATE as sql/02_seed_data.sql).
-- It lives in scripts/ (not sql/) so Docker never runs it automatically.
--
--   Load:    docker exec -i allotmate_postgres psql -U postgres -d allotmate < scripts/scale_seed_1500.sql
--   Restore: docker exec -i allotmate_postgres psql -U postgres -d allotmate < sql/02_seed_data.sql

TRUNCATE TABLE RUN_LOG, WAITLIST, ALLOTMENT, PREFERENCE, COORDINATOR, ELECTIVE, STUDENT RESTART IDENTITY CASCADE;

-- Coordinators (same bcrypt hash as the default seed: 'admin123')
INSERT INTO COORDINATOR (name, department, username, password_hash) VALUES
('Dr. Rajesh Sharma', 'Computer Science & Engineering', 'rsharma', '$2a$12$e8x/yUom7LgU4k5q3C3/jOXp5x5w9u2K7GqLzM1s9nFkUjV1WpP4G'),
('Dr. Priya Nair', 'Electronics & Communication', 'pnair', '$2a$12$e8x/yUom7LgU4k5q3C3/jOXp5x5w9u2K7GqLzM1s9nFkUjV1WpP4G');

-- 30 electives with uneven capacities (20..80 seats) so some are oversubscribed
-- and the matching has real competition to resolve.
INSERT INTO ELECTIVE (name, capacity, seats_filled, semester)
SELECT 'Elective ' || LPAD(j::text, 2, '0'),
       20 + ((j * 13) % 61),
       0,
       'Fall 2026'
FROM generate_series(1, 30) AS e(j);

-- 1,500 students across 4 branches with CGPAs spread over 6.00 - 9.99.
INSERT INTO STUDENT (name, roll_no, cgpa, branch, status)
SELECT 'Student ' || LPAD(i::text, 4, '0'),
       '2024SC' || LPAD(i::text, 4, '0'),
       ROUND((6.00 + (((i * 37) % 400)::numeric / 100.0))::numeric, 2),
       CASE (i % 4)
           WHEN 0 THEN 'Computer Science & Engineering'
           WHEN 1 THEN 'Information Technology'
           WHEN 2 THEN 'Artificial Intelligence & Data Science'
           ELSE 'Electronics & Communication Engineering'
       END,
       'UNASSIGNED'
FROM generate_series(1, 1500) AS s(i);

-- 5 preferences per student. Offsets 0, 6, 12, 18, 24 are distinct modulo 30,
-- so a student never ranks the same elective twice (UNIQUE constraint holds).
-- The (i % 7) term skews first choices so popularity is uneven.
INSERT INTO PREFERENCE (student_id, elective_id, rank)
SELECT s.i,
       ((((s.i * 7) + (s.i % 7)) % 30) + (k.k - 1) * 6) % 30 + 1,
       k.k
FROM generate_series(1, 1500) AS s(i)
CROSS JOIN generate_series(1, 5) AS k(k);

-- Refresh planner statistics so EXPLAIN reflects the new table sizes.
ANALYZE;
