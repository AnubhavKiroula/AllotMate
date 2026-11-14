#pragma once

#include <vector>
#include "Student.h"
#include "Elective.h"
#include "SparseMatrix.h"
#include "HashTable.h"
#include "Stack.h"
#include "PriorityQueue.h"

// FR-6: how fair one allocation is. The engine only computes the numbers;
// the CLI and the benchmark decide how to print them.
struct FairnessReport {
    int totalStudents = 0;
    int matched = 0;
    int unmatched = 0;
    int firstChoice = 0;              // students who got their rank-1 elective
    double firstChoicePercent = 0.0;  // firstChoice as a % of all students
    double averageRank = 0.0;         // average rank received by matched students (1 = best)
    int blockingPairs = 0;            // 0 means the allocation is stable
};

class MatchingEngine {

    private:
        Stack<Student> unmatchedStack;
        SparseMatrix prefMatrix;

        HashTable<int , Student> studentIndex;
        HashTable<int, Elective> electiveIndex;
        HashTable<int,int> nextPreferenceIndex;
        HashTable<int, std::vector<int>> electiveAssignments;

        std::vector<Student> students;
        std::vector<Elective> electives;

    public:
        MatchingEngine() = default;
        ~MatchingEngine() = default;

        void loadData(
            const std::vector<Student>& students,
            const std::vector<Elective>& electives
        );

        void runGaleShapley();
        int detectBlockingPairs() const;
        FairnessReport generateFairnessReport() const;

        const std::vector<Student>& getStudents() const;
        const std::vector<Elective>& getElectives() const;
        int getAssignedElectiveId(int studentId) const;

        // Elective id for each student, in getStudents() order (-1 = unmatched).
        std::vector<int> getAssignedElectiveIds() const;

        // One copy of every elective with its AVL waitlist filled in (FR-7).
        // A student waits for every elective they ranked above the one they
        // got, or for all their choices if they are unmatched.
        std::vector<Elective> buildWaitlists() const;

        // The same checks for ANY assignment, where assignedElectiveIds[i] is
        // the elective of students[i] (-1 = unmatched). The FCFS baseline in
        // tools/benchmark_matching.cpp is judged by exactly these rules.
        static int countBlockingPairs(
            const std::vector<Student>& students,
            const std::vector<Elective>& electives,
            const std::vector<int>& assignedElectiveIds
        );
        static FairnessReport buildFairnessReport(
            const std::vector<Student>& students,
            const std::vector<Elective>& electives,
            const std::vector<int>& assignedElectiveIds
        );
};
