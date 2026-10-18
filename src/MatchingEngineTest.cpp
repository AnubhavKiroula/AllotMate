#include "MatchingEngine.h"

#include <iostream>
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

    return 0;
}