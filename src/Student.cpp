#include "Student.h"
#include "AllocationException.h"
#include <iostream>

using namespace std;

Student::Student(int id, const string& name, float cgpa)
    : cgpa(cgpa),
    status(Status::UNMATCHED){
    this->id = id;
    this->name = name;
}

void Student::addPreference(int electiveID){
    for (int existingId : preferenceList) {
        if (existingId == electiveID) {
            throw AllocationException::InvalidPreferenceException(
                "Duplicate elective preference: elective ID already ranked."
            );
        }
    }
    preferenceList.push_back(electiveID);
}

void Student::displayProfile(){
    cout << "Student ID: " << id << endl;
    cout << "Name :" << name << endl;
    cout << "CGPA : " << cgpa << endl;
}
bool Student::operator<(const Student& other) const {

    if (cgpa != other.cgpa) {
        return cgpa < other.cgpa;
    }
    return id > other.id;
}

bool Student::operator==(const Student& other) const {
    return id == other.id;
}

float Student::getCgpa() const{
    return cgpa;
}

const vector<int>& Student::getPreferenceList() const {
    return preferenceList;
}

Status Student::getStatus() const{
    return status;
}

void Student::setStatus(Status newStatus){
    status = newStatus;
}