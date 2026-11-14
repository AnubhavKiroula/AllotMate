#include "MatchingEngine.h"

#include <iostream>
#include <utility>
#include <vector>

int main() {
    Student s1(101, "Aman", 9.5f);
    s1.addPreference(10);
    s1.addPreference(20);

    Student s2(102, "Riya", 8.5f);
    s2.addPreference(10);
    s2.addPreference(20);

    Student s3(103, "Neha", 9.0f);
    s3.addPreference(10);

    Elective e1(10, "AI", 1);
    Elective e2(20, "Data Science", 1);

    MatchingEngine engine;

    engine.loadData(
        std::vector<Student>{s1, s2, s3},
        std::vector<Elective>{e1, e2}
    );

    // First allocation run.
    engine.runGaleShapley();

    int ALLOTED = 0;
    int unmatched = 0;

    for (const Student& student : engine.getStudents()) {
        std::cout << student.getId() << ": "
                  << (student.getStatus() == Status::ALLOTED
                          ? "ALLOTED"
                          : "UNMATCHED")
                  << '\n';

        if (student.getStatus() == Status::ALLOTED) {
            ++ALLOTED;
        } else {
            ++unmatched;
        }
    }

    // Check elective capacities.
    for (const Elective& elective : engine.getElectives()) {
        std::cout << elective.getName() << ": "
                  << elective.getSeatsFilled() << "/"
                  << elective.getCapacity() << '\n';

        if (elective.getSeatsFilled() > elective.getCapacity()) {
            std::cerr << "FAIL: Capacity exceeded\n";
            return 1;
        }
    }

    if (ALLOTED != 2 || unmatched != 1) {
        std::cerr << "FAIL: Unexpected allocation counts\n";
        return 1;
    }

    std::cout << "Basic matching test: passed\n";

    // Second allocation run.
    engine.runGaleShapley();

    int ALLOTEDAfterSecondRun = 0;

    for (const Student& student : engine.getStudents()) {
        if (student.getStatus() == Status::ALLOTED) {
            ++ALLOTEDAfterSecondRun;
        }
    }

    std::cout << "ALLOTED after second run: "
              << ALLOTEDAfterSecondRun << '\n';

    if (ALLOTEDAfterSecondRun != 2) {
        std::cerr << "FAIL: Repeated run produced incorrect results\n";
        return 1;
    }

    // Verify capacities after the second run as well.
    for (const Elective& elective : engine.getElectives()) {
        if (elective.getSeatsFilled() > elective.getCapacity()) {
            std::cerr << "FAIL: Capacity exceeded after second run\n";
            return 1;
        }
    }

    std::cout << "Repeated-run test: passed\n";

        // Test zero-capacity elective.
    Student s4(104, "Karan", 8.7f);
    s4.addPreference(30);
    s4.addPreference(20);

    Elective e3(30, "Robotics", 0);
    Elective e4(20, "Cloud Computing", 1);

    MatchingEngine zeroCapacityEngine;
    zeroCapacityEngine.loadData(
        std::vector<Student>{s4},
        std::vector<Elective>{e3, e4}
    );

    zeroCapacityEngine.runGaleShapley();

    const Student& zeroCapacityStudent =
        zeroCapacityEngine.getStudents().front();

    if (zeroCapacityStudent.getStatus() != Status::ALLOTED) {
        std::cerr << "FAIL: Student should get the second preference\n";
        return 1;
    }

    for (const Elective& elective : zeroCapacityEngine.getElectives()) {
        if (elective.getId() == 30 && elective.getSeatsFilled() != 0) {
            std::cerr << "FAIL: Zero-capacity elective received a student\n";
            return 1;
        }
    }

    std::cout << "Zero-capacity test: passed\n";

        // Test exhausted preferences.
    Student s5(105, "Meera", 8.2f);
    s5.addPreference(40);

    Elective e5(40, "Cybersecurity", 0);

    MatchingEngine exhaustedEngine;
    exhaustedEngine.loadData(
        std::vector<Student>{s5},
        std::vector<Elective>{e5}
    );

    exhaustedEngine.runGaleShapley();

    const Student& exhaustedStudent =
        exhaustedEngine.getStudents().front();

    if (exhaustedStudent.getStatus() != Status::UNMATCHED) {
        std::cerr << "FAIL: Student should remain unmatched\n";
        return 1;
    }

    if (exhaustedEngine.getElectives().front().getSeatsFilled() != 0) {
        std::cerr << "FAIL: Zero-capacity elective received a student\n";
        return 1;
    }

    std::cout << "Exhausted-preferences test: passed\n";

        // Test displacement and reallocation.
    Student s6(201, "Aarav", 7.5f);
    s6.addPreference(50);
    s6.addPreference(60);

    Student s7(202, "Diya", 9.5f);
    s7.addPreference(50);

    Elective e6(50, "Machine Learning", 1);
    Elective e7(60, "Database Systems", 1);

    MatchingEngine displacementEngine;
    displacementEngine.loadData(
        std::vector<Student>{s7, s6},
        std::vector<Elective>{e6, e7}
    );

    displacementEngine.runGaleShapley();

    if(displacementEngine.getAssignedElectiveId(201) != 60 || displacementEngine.getAssignedElectiveId(202) != 50){
        std::cerr << "FAIL : Displaced student did not get reallocated\n";
        return 1;
    }

    std::cout << "Displacement and reallocation test: passed\n";

        // Test deterministic tie-breaking.
    Student s8(301, "Ishita", 8.5f);
    s8.addPreference(70);

    Student s9(302, "Kabir", 8.5f);
    s9.addPreference(70);

    Elective e8(70, "Artificial Intelligence", 1);

    MatchingEngine tieBreakEngine;
    tieBreakEngine.loadData(
        std::vector<Student>{s8, s9},
        std::vector<Elective>{e8}
    );

    tieBreakEngine.runGaleShapley();

    bool lowerIdAllotted = false;
    bool higherIdAllotted = false;

    for (const Student& student : tieBreakEngine.getStudents()) {
        if (student.getId() == 301 &&
            student.getStatus() == Status::ALLOTED) {
            lowerIdAllotted = true;
        }

        if (student.getId() == 302 &&
            student.getStatus() == Status::ALLOTED) {
            higherIdAllotted = true;
        }
    }

    if (!lowerIdAllotted || higherIdAllotted) {
        std::cerr << "FAIL: Lower-ID tie-break was not respected\n";
        return 1;
    }

    std::cout << "Deterministic tie-breaking test: passed\n";

    // Test HashTable growth: start at the default 101 slots and insert far
    // more keys, so the table must rehash several times.
    HashTable<int, int> table;
    table.insert(1, 100);
    int& firstValue = table.find(1); // reference taken BEFORE any growth

    for (int key = 2; key <= 1000; ++key) {
        table.insert(key, key * 100);
    }

    if (table.size() != 1000) {
        std::cerr << "FAIL: HashTable lost entries while growing\n";
        return 1;
    }

    for (int key = 1; key <= 1000; ++key) {
        if (!table.contains(key) || table.find(key) != key * 100) {
            std::cerr << "FAIL: HashTable key " << key << " wrong after rehash\n";
            return 1;
        }
    }

    // Rehash moves value pointers instead of copying values, so a reference
    // taken earlier must still point at the live value.
    firstValue = 7;
    if (table.find(1) != 7) {
        std::cerr << "FAIL: reference from find() went stale after rehash\n";
        return 1;
    }

    std::cout << "HashTable growth (rehash) test: passed\n";

    // Test the engine past the old 101-entry limit: 1,500 students, which
    // used to fail with "HashTable is full".
    std::vector<Student> manyStudents;
    for (int id = 1; id <= 1500; ++id) {
        Student student(id, "Student", 6.0f + (id % 400) / 100.0f);
        student.addPreference(1 + (id % 30));
        student.addPreference(1 + ((id + 10) % 30));
        student.addPreference(1 + ((id + 20) % 30));
        manyStudents.push_back(student);
    }

    std::vector<Elective> manyElectives;
    for (int id = 1; id <= 30; ++id) {
        manyElectives.push_back(Elective(id, "Elective", 40));
    }

    MatchingEngine largeEngine;
    try {
        largeEngine.loadData(manyStudents, manyElectives);
        largeEngine.runGaleShapley();
    } catch (const std::exception& e) {
        std::cerr << "FAIL: 1,500-student run threw: " << e.what() << '\n';
        return 1;
    }

    int largeAllotted = 0;
    for (const Student& student : largeEngine.getStudents()) {
        if (student.getStatus() == Status::ALLOTED) {
            ++largeAllotted;
        }
    }

    int seatsFilled = 0;
    for (const Elective& elective : largeEngine.getElectives()) {
        if (elective.getSeatsFilled() > elective.getCapacity()) {
            std::cerr << "FAIL: Capacity exceeded in large run\n";
            return 1;
        }
        seatsFilled += elective.getSeatsFilled();
    }

    // 30 electives x 40 seats = 1,200 seats for 1,500 students.
    if (largeAllotted != seatsFilled || largeAllotted != 1200) {
        std::cerr << "FAIL: Large run allotted " << largeAllotted
                  << " students, seats filled " << seatsFilled << '\n';
        return 1;
    }

    std::cout << "1,500-student scale test: passed (" << largeAllotted
              << " allotted, " << (1500 - largeAllotted) << " unmatched)\n";

    // Test stability: a Gale-Shapley result never has a blocking pair.
    if (engine.detectBlockingPairs() != 0 || largeEngine.detectBlockingPairs() != 0) {
        std::cerr << "FAIL: Gale-Shapley result has blocking pairs\n";
        return 1;
    }

    std::cout << "No-blocking-pairs test: passed\n";

    // Test the fairness report of the first engine: Aman (9.5) gets AI
    // (rank 1), Riya (8.5) gets Data Science (rank 2), Neha (9.0) listed
    // only AI and stays unmatched.
    FairnessReport report = engine.generateFairnessReport();
    if (report.totalStudents != 3 || report.matched != 2 || report.unmatched != 1 ||
        report.firstChoice != 1 || report.averageRank != 1.5 || report.blockingPairs != 0) {
        std::cerr << "FAIL: Fairness report numbers are wrong\n";
        return 1;
    }

    std::cout << "Fairness report test: passed\n";

    // Test waitlists for the same result: Riya got Data Science (her rank 2)
    // and Neha got nothing, so both wait for AI, Neha (9.0) ahead of Riya
    // (8.5). Aman got his first choice, so nobody waits for Data Science.
    for (const Elective& elective : engine.buildWaitlists()) {
        std::vector<int> expected;
        if (elective.getId() == 10) {
            expected = {103, 102};
        }
        if (elective.waitlistOrder() != expected) {
            std::cerr << "FAIL: Wrong waitlist for elective " << elective.getId() << '\n';
            return 1;
        }
    }

    std::cout << "Waitlist test: passed\n";

    // Test countBlockingPairs on an unfair assignment, like FCFS can produce:
    // Riya (8.5) took AI first, so Aman (9.5) ended up in Data Science.
    //   (Aman, AI) blocks: Aman prefers AI, and AI prefers Aman to Riya.
    //   (Neha, AI) blocks: Neha is unmatched, and AI prefers Neha to Riya.
    std::vector<Student> trio = {s1, s2, s3};      // Aman, Riya, Neha
    std::vector<Elective> twoElectives = {e1, e2}; // AI (10), Data Science (20)
    std::vector<int> unfairAssignment = {20, 10, -1};

    if (MatchingEngine::countBlockingPairs(trio, twoElectives, unfairAssignment) != 2) {
        std::cerr << "FAIL: Expected 2 blocking pairs in the unfair assignment\n";
        return 1;
    }

    // A free seat blocks for an unmatched student who listed it, but a
    // zero-capacity elective never forms a blocking pair (SRS #3).
    Student s10(401, "Zoya", 8.0f);
    s10.addPreference(80);
    s10.addPreference(90);

    std::vector<Student> zoya = {s10};
    std::vector<Elective> zeroAndOpen = {Elective(80, "Zero Seats", 0), Elective(90, "Open Seats", 2)};

    if (MatchingEngine::countBlockingPairs(zoya, zeroAndOpen, std::vector<int>{-1}) != 1 ||
        MatchingEngine::countBlockingPairs(zoya, zeroAndOpen, std::vector<int>{90}) != 0) {
        std::cerr << "FAIL: Free-seat / zero-capacity blocking pair rules\n";
        return 1;
    }

    std::cout << "Blocking-pair counting test: passed\n";

    // Test Stack: a moved-from stack has capacity 0 and must still grow.
    Stack<int> original;
    original.push(1);
    Stack<int> movedTo(std::move(original));
    original.push(2);

    if (original.size() != 1 || original.top() != 2 || movedTo.top() != 1) {
        std::cerr << "FAIL: Stack push after move\n";
        return 1;
    }

    std::cout << "Stack push-after-move test: passed\n";

    return 0;
}
