#pragma once

#include <vector>
#include "Student.h"
#include "Elective.h"
#include "SparseMatrix.h"
#include "HashTable.h"
#include "Stack.h"
#include "PriorityQueue.h"

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
        void detectBlockingPairs();
        void generateFairnessReport();

        const std::vector<Student>& getStudents() const;
        const std::vector<Elective>& getElectives() const;
        int getAssignedElectiveId(int studentId) const;
};