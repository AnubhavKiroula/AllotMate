#pragma once

#include <vector>

struct SparseEntry {
    int studentId;
    int electiveId;
    int preferenceRank;
};

class SparseMatrix {
    private:
        std::vector<SparseEntry> entries;

    public:
        SparseMatrix() = default;
        ~SparseMatrix() = default;

        void insert(int studentId, int electiveId, int preferenceRank);
        int get(int studentId, int electiveId) const;
        bool contains(int studentId, int electiveId) const;
        int size() const;
};