#pragma once

#include <string>
using namespace std;

class Elective{

    private:
        int id;
        string name;
        int capacity;
        int seatsFilled=0;

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
};