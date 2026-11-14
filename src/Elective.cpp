#include "Elective.h"
#include "AllocationException.h"
using namespace std;


Elective::Elective(
    int id,
    const string& name,
    int capacity
)
    : id(id),
      name(name),
      capacity(capacity){

      }
void Elective::allocateSeat() {
    if (seatsFilled >= capacity) {
        throw AllocationException::SeatOverflowException(
            "Cannot allocate seat: elective is at full capacity."
        );
    }
    seatsFilled++;
}

bool Elective::hasSeat() const {
    return seatsFilled < capacity;
}


int Elective::getId() const {
    return id;
}

const string& Elective::getName() const{
    return name;
}

int Elective::getCapacity() const {
    return capacity;
}

int Elective::getSeatsFilled() const {
    return seatsFilled;
}

bool Elective::operator<(const Elective& other) const {
    return capacity < other.capacity;
}

bool Elective::operator==(const Elective& other) const {
    return id == other.id;
}

void Elective::resetSeats(){
    seatsFilled = 0;
}
void Elective::addToWaitlist(const Student& s) {
    waitlist.insert(s);
}

void Elective::releaseSeat() {
    if (seatsFilled > 0) --seatsFilled;
}

// Gives a free seat to the highest-priority waitlisted student.
bool Elective::promoteNext(Student& promoted) {
    if (!hasSeat()) return false;
    if (!waitlist.extractMax(promoted)) return false;
    allocateSeat();
    return true;
}

int Elective::waitlistSize() const {
    return waitlist.size();
}

// The AVL in-order walk gives the LOWEST priority first, so it is read
// backwards: index 0 is the student who gets the next free seat.
vector<int> Elective::waitlistOrder() const {
    vector<int> lowestFirst;
    waitlist.inorder([&](const Student& s) { lowestFirst.push_back(s.getId()); });

    vector<int> frontFirst;
    for (int i = static_cast<int>(lowestFirst.size()) - 1; i >= 0; i--) {
        frontFirst.push_back(lowestFirst[i]);
    }
    return frontFirst;
}