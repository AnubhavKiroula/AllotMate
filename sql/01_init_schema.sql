-- AllotMate Database Schema Migration
-- Matches ER diagram in docs/00_PROJECT-OVERVIEW-AND-DIAGRAMS.md and specs in docs/02_SRS.md, docs/03_ARCHITECTURE.md

-- Drop existing tables in reverse dependency order if recreating
DROP TABLE IF EXISTS RUN_LOG CASCADE;
DROP TABLE IF EXISTS WAITLIST CASCADE;
DROP TABLE IF EXISTS ALLOTMENT CASCADE;
DROP TABLE IF EXISTS PREFERENCE CASCADE;
DROP TABLE IF EXISTS COORDINATOR CASCADE;
DROP TABLE IF EXISTS ELECTIVE CASCADE;
DROP TABLE IF EXISTS STUDENT CASCADE;

-- 1. STUDENT Table
CREATE TABLE STUDENT (
    student_id SERIAL PRIMARY KEY,
    name VARCHAR(100) NOT NULL,
    roll_no VARCHAR(20) NOT NULL,
    cgpa NUMERIC(4, 2) NOT NULL CHECK (cgpa >= 0.00 AND cgpa <= 10.00),
    branch VARCHAR(50) NOT NULL,
    status VARCHAR(20) NOT NULL DEFAULT 'UNASSIGNED',
    CONSTRAINT uk_student_roll_no UNIQUE (roll_no)
);

-- 2. ELECTIVE Table
CREATE TABLE ELECTIVE (
    elective_id SERIAL PRIMARY KEY,
    name VARCHAR(150) NOT NULL,
    capacity INT NOT NULL CHECK (capacity >= 0),
    seats_filled INT NOT NULL DEFAULT 0 CHECK (seats_filled >= 0 AND seats_filled <= capacity),
    semester VARCHAR(20) NOT NULL
);

-- 3. COORDINATOR Table
CREATE TABLE COORDINATOR (
    coordinator_id SERIAL PRIMARY KEY,
    name VARCHAR(100) NOT NULL,
    department VARCHAR(50) NOT NULL,
    username VARCHAR(50) NOT NULL,
    password_hash VARCHAR(255) NOT NULL,
    CONSTRAINT uk_coordinator_username UNIQUE (username)
);

-- 4. PREFERENCE Table
CREATE TABLE PREFERENCE (
    pref_id SERIAL PRIMARY KEY,
    student_id INT NOT NULL,
    elective_id INT NOT NULL,
    rank INT NOT NULL CHECK (rank > 0),
    CONSTRAINT fk_preference_student FOREIGN KEY (student_id) REFERENCES STUDENT(student_id) ON DELETE CASCADE,
    CONSTRAINT fk_preference_elective FOREIGN KEY (elective_id) REFERENCES ELECTIVE(elective_id) ON DELETE RESTRICT,
    CONSTRAINT uk_preference_student_elective UNIQUE (student_id, elective_id),
    CONSTRAINT uk_preference_student_rank UNIQUE (student_id, rank)
);

-- 5. ALLOTMENT Table
CREATE TABLE ALLOTMENT (
    allotment_id SERIAL PRIMARY KEY,
    student_id INT NOT NULL,
    elective_id INT NOT NULL,
    allotted_on TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,
    round VARCHAR(20) NOT NULL DEFAULT 'Round 1',
    CONSTRAINT fk_allotment_student FOREIGN KEY (student_id) REFERENCES STUDENT(student_id) ON DELETE CASCADE,
    CONSTRAINT fk_allotment_elective FOREIGN KEY (elective_id) REFERENCES ELECTIVE(elective_id) ON DELETE RESTRICT,
    CONSTRAINT uk_allotment_student_round UNIQUE (student_id, round)
);

-- 6. WAITLIST Table
CREATE TABLE WAITLIST (
    waitlist_id SERIAL PRIMARY KEY,
    student_id INT NOT NULL,
    elective_id INT NOT NULL,
    position INT NOT NULL CHECK (position > 0),
    CONSTRAINT fk_waitlist_student FOREIGN KEY (student_id) REFERENCES STUDENT(student_id) ON DELETE CASCADE,
    CONSTRAINT fk_waitlist_elective FOREIGN KEY (elective_id) REFERENCES ELECTIVE(elective_id) ON DELETE RESTRICT,
    CONSTRAINT uk_waitlist_student_elective UNIQUE (student_id, elective_id),
    CONSTRAINT uk_waitlist_elective_position UNIQUE (elective_id, position)
);

-- 7. RUN_LOG Table
CREATE TABLE RUN_LOG (
    run_id SERIAL PRIMARY KEY,
    coordinator_id INT,
    run_at TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,
    blocking_pairs_found INT,  -- NULL = stability check not run for this run (never assume 0)
    outcome_summary TEXT,
    CONSTRAINT fk_run_log_coordinator FOREIGN KEY (coordinator_id) REFERENCES COORDINATOR(coordinator_id) ON DELETE SET NULL
);

-- Required Indexes (per 00_PROJECT-OVERVIEW-AND-DIAGRAMS.md and 03_ARCHITECTURE.md)
CREATE INDEX idx_student_roll_no ON STUDENT(roll_no);
CREATE INDEX idx_preference_student_rank ON PREFERENCE(student_id, rank);
CREATE INDEX idx_allotment_elective_id ON ALLOTMENT(elective_id);
