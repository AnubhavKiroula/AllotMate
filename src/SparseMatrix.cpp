#include "SparseMatrix.h"

void SparseMatrix::insert(
    int studentId,
    int electiveId,
    int preferenceRank
) {
    for (SparseEntry& entry : entries) {
        if(entry.studentId == studentId &&
           entry.electiveId == electiveId){

            entry.preferenceRank = preferenceRank;
            return;
        }
    }

    entries.push_back({
        studentId,
        electiveId,
        preferenceRank,
    });
}

int SparseMatrix::get(
    int studentId,
    int electiveId
) const {
    for(const SparseEntry& entry : entries) {
        if(entry.studentId == studentId &&
            entry.electiveId == electiveId) {

                return entry.preferenceRank;
        }
    }
    return 0;
}

bool SparseMatrix::contains(
    int studentId,
    int electiveId
) const {
    for(const SparseEntry& entry : entries){
        if(entry.studentId == studentId &&
            entry.electiveId == electiveId) {
                
                return true;
        }
    }
    return false;
}

int SparseMatrix::size() const {
    return static_cast<int>(entries.size());
}