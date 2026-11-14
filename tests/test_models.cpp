// ============================================================
// test_models.cpp -- unit tests for the models and the trees (no database)
//
//   1. Student / Elective edge cases from the SRS (#2, #3, #8),
//      seat overflow and waitlist promotion.
//   2. AVLTree:     stays balanced, remove, extractMax order, Rule of 5.
//   3. ThreadedBST: stack-less inorder is sorted, copies are independent.
//
// Uses the same check() macro as test_csv_importer.cpp instead of assert():
// CI builds in Release, where assert() is compiled out.
// ============================================================

#include <iostream>
#include <utility>
#include <vector>

#include "AVLTree.h"
#include "AllocationException.h"
#include "Elective.h"
#include "Student.h"
#include "ThreadedBST.h"

static int g_failures = 0;

#define check(cond, msg)                                           \
    do {                                                           \
        if (!(cond)) {                                             \
            std::cout << "  [FAIL] " << (msg) << "\n";             \
            g_failures++;                                          \
        } else {                                                   \
            std::cout << "  [ok]   " << (msg) << "\n";             \
        }                                                          \
    } while (0)

// Collects a tree's inorder walk into a vector so it can be compared.
template <typename Tree>
static std::vector<int> inorderOf(const Tree& tree) {
    std::vector<int> out;
    tree.inorder([&](int x) { out.push_back(x); });
    return out;
}

// ---------------- Student / Elective ----------------

static void testDuplicatePreferenceIsRejected() {
    std::cout << "testDuplicatePreferenceIsRejected (SRS #2)\n";
    Student s(1, "A", 8.0f);
    s.addPreference(10);
    bool threw = false;
    try {
        s.addPreference(10);
    } catch (const AllocationException::InvalidPreferenceException&) {
        threw = true;
    }
    check(threw, "adding the same elective twice throws InvalidPreferenceException");
    check(s.getPreferenceList().size() == 1, "the duplicate is not stored");
}

static void testCgpaTieIsBrokenById() {
    std::cout << "testCgpaTieIsBrokenById (SRS #8)\n";
    Student a(101, "A", 9.0f);
    Student b(102, "B", 9.0f);
    Student c(103, "C", 8.0f);
    check(b < a, "same CGPA: the higher id has lower priority");
    check(!(a < b), "same CGPA: the lower id is not below the higher id");
    check(c < a, "lower CGPA has lower priority");
}

static void testStudentEqualityIsById() {
    std::cout << "testStudentEqualityIsById\n";
    Student a(5, "X", 9.0f);
    Student b(5, "Y", 7.0f);
    check(a == b, "two students with the same id are equal");
}

static void testZeroCapacityElectiveNeverHasSeat() {
    std::cout << "testZeroCapacityElectiveNeverHasSeat (SRS #3)\n";
    Elective e(1, "Zero", 0);
    check(!e.hasSeat(), "a zero-capacity elective has no seat");

    e.addToWaitlist(Student(7, "W", 9.0f));
    Student promoted(0, "", 0.0f);
    check(!e.promoteNext(promoted), "nobody is promoted into a zero-capacity elective");
    check(e.waitlistSize() == 1, "the waitlisted student stays on the waitlist");
}

static void testSeatOverflowThrows() {
    std::cout << "testSeatOverflowThrows\n";
    Elective e(2, "One", 1);
    e.allocateSeat();
    check(!e.hasSeat(), "the only seat is taken");
    bool threw = false;
    try {
        e.allocateSeat();
    } catch (const AllocationException::SeatOverflowException&) {
        threw = true;
    }
    check(threw, "allocating past capacity throws SeatOverflowException");
    check(e.getSeatsFilled() == 1, "seatsFilled never goes above capacity");
}

static void testWaitlistPromotesHighestPriorityFirst() {
    std::cout << "testWaitlistPromotesHighestPriorityFirst\n";
    Elective e(3, "W", 1);
    e.allocateSeat();
    e.addToWaitlist(Student(104, "D", 9.0f));
    e.addToWaitlist(Student(101, "A", 9.0f));

    Student promoted(0, "", 0.0f);
    check(!e.promoteNext(promoted), "no promotion while the elective is full");

    e.releaseSeat();
    check(e.promoteNext(promoted), "a released seat is given to the waitlist");
    check(promoted.getId() == 101, "tie on CGPA: the lower id (101) is promoted first");
    check(e.waitlistSize() == 1, "the promoted student leaves the waitlist");
    check(!e.hasSeat(), "the promoted student takes the free seat");
}

static void testWaitlistOrderIsFrontFirst() {
    std::cout << "testWaitlistOrderIsFrontFirst\n";
    Elective e(4, "Order", 0);
    e.addToWaitlist(Student(7, "Kabir", 8.2f));
    e.addToWaitlist(Student(9, "Neel", 9.4f));
    e.addToWaitlist(Student(5, "Tara", 7.9f));
    e.addToWaitlist(Student(3, "Isha", 9.4f));
    check(e.waitlistOrder() == std::vector<int>({3, 9, 7, 5}),
          "waitlistOrder is highest priority first (CGPA tie: lower id first)");
    check(e.waitlistSize() == 4, "reading the order removes nobody");
}

// ---------------- AVLTree ----------------

static void testAvlStaysBalancedOnSortedInput() {
    std::cout << "testAvlStaysBalancedOnSortedInput\n";
    AVLTree<int> tree;
    for (int i = 1; i <= 7; i++) {
        tree.insert(i);   // sorted input: a plain BST would become a list of height 7
    }
    check(tree.size() == 7, "size is 7");
    check(tree.treeHeight() == 3, "height is 3 (rotations kept it balanced)");
    check(inorderOf(tree) == std::vector<int>({1, 2, 3, 4, 5, 6, 7}), "inorder is sorted");
}

static void testAvlRemove() {
    std::cout << "testAvlRemove\n";
    AVLTree<int> tree;
    int values[] = {50, 30, 70, 20, 40, 60, 80};
    for (int v : values) {
        tree.insert(v);
    }
    check(!tree.remove(99), "removing a missing value returns false");
    check(tree.size() == 7, "size is unchanged after a failed remove");

    check(tree.remove(30), "removing a node with two children returns true");
    check(tree.size() == 6, "size drops to 6");
    check(inorderOf(tree) == std::vector<int>({20, 40, 50, 60, 70, 80}), "inorder is still sorted");
}

static void testAvlExtractMaxOrder() {
    std::cout << "testAvlExtractMaxOrder\n";
    AVLTree<int> tree;
    int values[] = {5, 1, 9, 3, 7};
    for (int v : values) {
        tree.insert(v);
    }
    std::vector<int> taken;
    int top = 0;
    while (tree.extractMax(top)) {
        taken.push_back(top);
    }
    check(taken == std::vector<int>({9, 7, 5, 3, 1}), "extractMax returns values from largest to smallest");
    check(tree.empty() && tree.size() == 0, "the tree is empty afterwards");
    check(!tree.extractMax(top), "extractMax on an empty tree returns false");
}

static void testAvlCopyAndMove() {
    std::cout << "testAvlCopyAndMove (Rule of 5)\n";
    AVLTree<int> original;
    original.insert(1);
    original.insert(2);

    AVLTree<int> copy(original);
    original.insert(3);
    check(copy.size() == 2, "a copy is a deep copy: it does not see later inserts");
    check(inorderOf(copy) == std::vector<int>({1, 2}), "the copy keeps its own values");

    AVLTree<int> moved(std::move(original));
    check(moved.size() == 3, "move constructor takes all nodes");
    check(original.empty(), "the moved-from tree is left empty");

    copy = moved;
    check(inorderOf(copy) == std::vector<int>({1, 2, 3}), "copy assignment replaces the old contents");
}

// ---------------- ThreadedBST ----------------

static void testThreadedInorderIsSorted() {
    std::cout << "testThreadedInorderIsSorted\n";
    ThreadedBST<int> tree;
    int values[] = {40, 20, 60, 10, 30, 50, 70};
    for (int v : values) {
        tree.insert(v);
    }
    check(tree.size() == 7, "size is 7");
    check(inorderOf(tree) == std::vector<int>({10, 20, 30, 40, 50, 60, 70}),
          "stack-less inorder visits values in sorted order");
}

static void testThreadedCopyIsIndependent() {
    std::cout << "testThreadedCopyIsIndependent (Rule of 5)\n";
    ThreadedBST<int> original;
    original.insert(2);
    original.insert(1);

    ThreadedBST<int> copy(original);
    original.insert(3);
    check(inorderOf(copy) == std::vector<int>({1, 2}), "a copy does not see later inserts");
    check(inorderOf(original) == std::vector<int>({1, 2, 3}), "the original keeps its own insert");

    ThreadedBST<int> moved(std::move(original));
    check(moved.size() == 3 && original.empty(), "move leaves the source empty");
}

int main() {
    std::cout << "=== AllotMate model + tree unit tests ===\n";
    testDuplicatePreferenceIsRejected();
    testCgpaTieIsBrokenById();
    testStudentEqualityIsById();
    testZeroCapacityElectiveNeverHasSeat();
    testSeatOverflowThrows();
    testWaitlistPromotesHighestPriorityFirst();
    testWaitlistOrderIsFrontFirst();

    testAvlStaysBalancedOnSortedInput();
    testAvlRemove();
    testAvlExtractMaxOrder();
    testAvlCopyAndMove();

    testThreadedInorderIsSorted();
    testThreadedCopyIsIndependent();

    std::cout << "=========================================\n";
    if (g_failures == 0) {
        std::cout << "All checks passed.\n";
        return 0;
    }
    std::cout << g_failures << " check(s) failed.\n";
    return 1;
}
