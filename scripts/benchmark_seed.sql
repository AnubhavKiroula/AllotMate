-- AllotMate Benchmark Dataset Generator (input for tools/benchmark_matching.cpp)
--
-- WARNING: this REPLACES all data (same TRUNCATE as sql/02_seed_data.sql).
-- It lives in scripts/ (not sql/) so Docker never runs it automatically:
-- it needs the student_count variable, which Docker cannot pass.
--
--   Load:    docker exec -i allotmate_postgres psql -U postgres -d allotmate -v student_count=1500 < scripts/benchmark_seed.sql
--   Restore: docker exec -i allotmate_postgres psql -U postgres -d allotmate < sql/02_seed_data.sql

\set ON_ERROR_STOP on

-- Validate input (stops here, before the TRUNCATE, if student_count is missing)
SELECT :student_count AS requested_student_count;

TRUNCATE TABLE
    RUN_LOG,
    WAITLIST,
    ALLOTMENT,
    PREFERENCE,
    COORDINATOR,
    ELECTIVE,
    STUDENT
RESTART IDENTITY CASCADE;

-- Coordinators (same as the default seed). The CLI logs every run under
-- coordinator 1, and RUN_LOG has a foreign key to COORDINATOR.
INSERT INTO COORDINATOR (name, department, username, password_hash) VALUES
('Dr. Rajesh Sharma', 'Computer Science & Engineering', 'rsharma', '$2a$12$e8x/yUom7LgU4k5q3C3/jOXp5x5w9u2K7GqLzM1s9nFkUjV1WpP4G'),
('Dr. Priya Nair', 'Electronics & Communication', 'pnair', '$2a$12$e8x/yUom7LgU4k5q3C3/jOXp5x5w9u2K7GqLzM1s9nFkUjV1WpP4G');

-- Insert six electives.
-- Total capacity is approximately 80% of student count.
INSERT INTO ELECTIVE (name, capacity, seats_filled, semester)
VALUES
('Advanced Machine Learning', (:student_count * 15 / 100), 0, 'Fall 2026'),
('Cloud Computing & DevOps', (:student_count * 15 / 100), 0, 'Fall 2026'),
('Cryptography & Network Security', (:student_count * 14 / 100), 0, 'Fall 2026'),
('Natural Language Processing', (:student_count * 14 / 100), 0, 'Fall 2026'),
('Robotics & Computer Vision', (:student_count * 12 / 100), 0, 'Fall 2026'),
('Quantum Computing Fundamentals', (:student_count * 10 / 100), 0, 'Fall 2026');

-- Generate students with varied CGPAs and branches.
INSERT INTO STUDENT (name, roll_no, cgpa, branch, status)
SELECT
    'Student ' || LPAD(i::text, 4, '0'),
    'BENCH' || LPAD(i::text, 4, '0'),
    ROUND((6.00 + ((i * 37) % 400)::numeric / 100), 2),
    CASE (i % 4)
        WHEN 0 THEN 'Computer Science & Engineering'
        WHEN 1 THEN 'Information Technology'
        WHEN 2 THEN 'Artificial Intelligence & Data Science'
        ELSE 'Electronics & Communication Engineering'
    END,
    'UNASSIGNED'
FROM generate_series(1, :student_count) AS s(i);

-- Generate four distinct ranked preferences per student.
INSERT INTO PREFERENCE (student_id, elective_id, rank)
SELECT
    s.i,
    (((s.i + r.rank - 1) % 6) + 1),
    r.rank
FROM generate_series(1, :student_count) AS s(i)
CROSS JOIN generate_series(1, 4) AS r(rank);

-- Verify generated data.
SELECT
    (SELECT COUNT(*) FROM STUDENT) AS students,
    (SELECT COUNT(*) FROM ELECTIVE) AS electives,
    (SELECT COUNT(*) FROM PREFERENCE) AS preferences,
    (SELECT SUM(capacity) FROM ELECTIVE) AS total_capacity;
