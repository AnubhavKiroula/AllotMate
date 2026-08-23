#include "Student.h"
#include <iostream>

using namespace std;

Student::Student(int id, const string& name, float cgpa)
    : cgpa(cgpa),
    status(Status::UNMATCHED){
    this->id = id;
    this->name = name;
}

void Student::addPerference(int electiveID){
    preferenceList.push_back(electiveID);
}

void Student::displayProfile(){
    cout << "Studnet ID: " << id << endl;
    cout << "Name :" << name << endl;
    cout << "CGPA : " << cgpa << endl;
}

bool Student::operator<(const Student& other) const {
    return cgpa < other.cgpa;
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