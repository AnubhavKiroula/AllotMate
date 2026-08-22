-- AllotMate Dummy Seed Data Script (100 Students + Realistic Electives + Preferences)
-- Respects all foreign keys, unique constraints, and check constraints

-- Clean existing data
TRUNCATE TABLE RUN_LOG, WAITLIST, ALLOTMENT, PREFERENCE, COORDINATOR, ELECTIVE, STUDENT RESTART IDENTITY CASCADE;

-- 1. Insert Coordinators (Password: 'admin123' hashed with bcrypt)
INSERT INTO COORDINATOR (name, department, username, password_hash) VALUES
('Dr. Rajesh Sharma', 'Computer Science & Engineering', 'rsharma', '$2a$12$e8x/yUom7LgU4k5q3C3/jOXp5x5w9u2K7GqLzM1s9nFkUjV1WpP4G'),
('Dr. Priya Nair', 'Electronics & Communication', 'pnair', '$2a$12$e8x/yUom7LgU4k5q3C3/jOXp5x5w9u2K7GqLzM1s9nFkUjV1WpP4G');

-- 2. Insert Electives (Semester: 'Fall 2026')
INSERT INTO ELECTIVE (name, capacity, seats_filled, semester) VALUES
('Advanced Machine Learning', 25, 0, 'Fall 2026'),
('Cloud Computing & DevOps', 30, 0, 'Fall 2026'),
('Cryptography & Network Security', 20, 0, 'Fall 2026'),
('Natural Language Processing', 20, 0, 'Fall 2026'),
('Robotics & Computer Vision', 15, 0, 'Fall 2026'),
('Quantum Computing Fundamentals', 10, 0, 'Fall 2026');

-- 3. Insert 100 Students using generate_series
INSERT INTO STUDENT (name, roll_no, cgpa, branch, status)
SELECT
    'Student ' || LPAD(i::text, 3, '0') AS name,
    '2024CS' || LPAD(i::text, 3, '0') AS roll_no,
    ROUND((6.00 + ( ( (i * 37) % 400 )::numeric / 100.0) )::numeric, 2) AS cgpa,
    CASE (i % 4)
        WHEN 0 THEN 'Computer Science & Engineering'
        WHEN 1 THEN 'Information Technology'
        WHEN 2 THEN 'Artificial Intelligence & Data Science'
        ELSE 'Electronics & Communication Engineering'
    END AS branch,
    'UNASSIGNED' AS status
FROM generate_series(1, 100) AS s(i);

-- 4. Insert Preferences for each student (4 ranked preferences per student)
-- Elective assignment formula ensures distinct electives per student
INSERT INTO PREFERENCE (student_id, elective_id, rank)
SELECT 
    s.i AS student_id,
    (((s.i + r.rank - 1) % 6) + 1) AS elective_id,
    r.rank AS rank
FROM generate_series(1, 100) AS s(i)
CROSS JOIN generate_series(1, 4) AS r(rank);

-- 5. Insert Sample Initial Run Log
INSERT INTO RUN_LOG (coordinator_id, run_at, blocking_pairs_found, outcome_summary) VALUES
(1, CURRENT_TIMESTAMP - INTERVAL '1 day', 0, 'Initial baseline system test run. Seed dataset initialized.');
