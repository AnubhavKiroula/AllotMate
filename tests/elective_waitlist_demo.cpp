#include "Elective.h"
#include "Student.h"
#include <iostream>

int main() {
    Elective e(1, "Data Structures", 1);
    e.allocateSeat();                          // full

    e.addToWaitlist(Student(104, "Dev", 9.0f));
    e.addToWaitlist(Student(101, "Asha", 9.0f));
    e.addToWaitlist(Student(102, "Ravi", 8.5f));
    std::cout << "Waitlist size: " << e.waitlistSize() << " (expect 3)\n";

    Student p(0, "", 0.0f);
    std::cout << "Promote while full: " << e.promoteNext(p) << " (expect 0)\n";

    e.releaseSeat();
    std::cout << "Promote after release: " << e.promoteNext(p) << " (expect 1)\n";
    p.displayProfile();                        // expect Student ID: 101
    std::cout << "Waitlist size: " << e.waitlistSize() << " (expect 2)\n";
    return 0;
}