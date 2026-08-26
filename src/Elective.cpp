#include "Elective.h"
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

bool Elective::hasSeat() const {
    return capacity > 0;
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

bool Elective::operator<(const Elective& other) const {
    return capacity < other.capacity;
}

bool Elective::operator==(const Elective& other) const {
    return id == other.id;
}