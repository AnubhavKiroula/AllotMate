#pragma once

#include <vector>
#include "PostgresConnection.h"

// Persists per-elective waitlists to the WAITLIST table.
//
// The ORDER of a waitlist is decided in memory by the OOP module's AVLTree
// (its in-order traversal gives students front-of-line first). This class only
// stores and loads that order; it deliberately does not rank students itself,
// so the AVL tree stays the single place where waitlist ordering lives.
class WaitlistRepository {
private:
    PostgresConnection& db;

public:
    explicit WaitlistRepository(PostgresConnection& connection);

    // Replaces an elective's whole waitlist with `orderedStudentIds`
    // (index 0 -> position 1). Runs in ONE transaction, so if anything fails
    // (e.g. the same student listed twice) the old waitlist is left untouched.
    void replaceWaitlist(int electiveId, const std::vector<int>& orderedStudentIds);

    // Returns the student ids waiting for an elective, front of the line first.
    std::vector<int> loadWaitlist(int electiveId);
};
