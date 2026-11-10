#include "Student.h"
#include "Elective.h"
#include "AllocationException.h"
#include <cassert>
#include <iostream>

int main() {
    // SRS #2: duplicate preference is rejected
    {
        Student s(1, "A", 8.0f);
        s.addPreference(10);
        bool threw = false;
        try { s.addPreference(10); }
        catch (const AllocationException::InvalidPreferenceException&) { threw = true; }
        assert(threw);
    }

    // SRS #8: CGPA tie broken by id (lower id has higher priority)
    {
        Student a(101, "A", 9.0f), b(102, "B", 9.0f);
        assert(b < a);
        assert(!(a < b));
        Student c(103, "C", 8.0f);
        assert(c < a);
    }

    // equality is by id
    {
        Student a(5, "X", 9.0f), b(5, "Y", 7.0f);
        assert(a == b);
    }

    // SRS #3: zero-capacity elective never has a seat
    {
        Elective e(1, "Zero", 0);
        assert(!e.hasSeat());
        Student p(0, "", 0.0f);
        assert(!e.promoteNext(p));
    }

    // allocateSeat overflow throws
    {
        Elective e(2, "One", 1);
        e.allocateSeat();
        assert(!e.hasSeat());
        bool threw = false;
        try { e.allocateSeat(); }
        catch (const AllocationException::SeatOverflowException&) { threw = true; }
        assert(threw);
    }

    // waitlist promotes highest priority first
    {
        Elective e(3, "W", 1);
        e.allocateSeat();
        e.addToWaitlist(Student(104, "D", 9.0f));
        e.addToWaitlist(Student(101, "A", 9.0f));
        e.releaseSeat();
        Student p(0, "", 0.0f);
        assert(e.promoteNext(p));
        assert(p.getId() == 101);
        assert(e.waitlistSize() == 1);
    }

    std::cout << "All model tests passed\n";
    return 0;
}