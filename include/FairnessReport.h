#pragma once
#include "ThreadedBST.h"
#include "Student.h"
#include <iostream>
#include <vector>

// Prints students in merit order using the stack-less threaded traversal.
// Inorder gives ascending order (lowest merit first), so we collect and
// print in reverse to show rank 1 (highest CGPA, lower id on ties) first.
class FairnessReport {
public:
    static void print(const std::vector<Student>& students) {
        ThreadedBST<Student> tree;
        for (const Student& s : students) tree.insert(s);

        std::vector<Student> ordered;
        tree.inorder([&](const Student& s) { ordered.push_back(s); });

        std::cout << "=== Merit Ranking Report ===\n";
        int rank = 1;
        for (auto it = ordered.rbegin(); it != ordered.rend(); ++it, ++rank) {
            std::cout << "Rank " << rank << ": ID " << it->getId()
                      << " | CGPA " << it->getCgpa() << "\n";
        }
        std::cout << "Total students: " << tree.size() << "\n";
    }
};