-- AllotMate Benchmark Dataset Generator
-- Usage: psql -v student_count=100 -f benchmark_seed.sql

\set ON_ERROR_STOP on

-- Validate input
SELECT :student_count AS requested_student_count;

-- Clear benchmark database data only
TRUNCATE TABLE
    RUN_LOG,
    WAITLIST,
    ALLOTMENT,
    PREFERENCE,
    COORDINATOR,
    ELECTIVE,
    STUDENT
RESTART IDENTITY CASCADE;

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