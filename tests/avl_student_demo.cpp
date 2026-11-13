#include "AVLTree.h"
#include "Student.h"
#include <iostream>

int main() {
    AVLTree<Student> waitlist;
    waitlist.insert(Student(104, "Dev", 9.0f));
    waitlist.insert(Student(101, "Asha", 9.0f));
    waitlist.insert(Student(103, "Meera", 9.0f));
    waitlist.insert(Student(102, "Ravi", 8.5f));   // lower CGPA, must come last

    Student s(0, "", 0.0f);
    while (waitlist.extractMax(s)) {
        s.displayProfile();
    }
    return 0;
}