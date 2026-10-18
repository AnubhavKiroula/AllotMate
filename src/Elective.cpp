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