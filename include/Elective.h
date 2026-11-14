#pragma once

#include <string>
#include <vector>
#include "AVLTree.h"
#include "Student.h"
using namespace std;

class Elective{

    private:
        int id;
        string name;
        int capacity;
        int seatsFilled=0;
        AVLTree<Student> waitlist;
        
    public:
        Elective(int id, const string& name, int capacity);

        bool hasSeat() const;

        int getId() const;
        const string& getName() const;
        int getCapacity() const;
        int getSeatsFilled() const;
        
        bool operator<(const Elective& other) const;
        bool operator==(const Elective& other) const;
        void allocateSeat();
        void resetSeats();
        void addToWaitlist(const Student& s);
        bool promoteNext(Student& promoted);
        int waitlistSize() const;
        vector<int> waitlistOrder() const; // student ids, front of the line first
        void releaseSeat();
};