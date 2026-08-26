#include "Coordinator.h"
#include "Elective.h"
#include <iostream>

using namespace std;

Coordinator::Coordinator(
    int id,
    const string& name,
    const string& department
)
    : department(department){
        this->id = id;
        this->name = name;
}

void Coordinator::approveAllocation(Elective& elective){
    cout << "Coordinator " << name << " approved allocation for elective: "
         << elective.getName() << endl;
}

void Coordinator::displayProfile(){
    cout << "Coordinator ID: " << id << endl;
    cout << "Name: " << name << endl;
    cout << "Department: " << department << endl;
}

const string& Coordinator::getDepartment() const{
    return department;
}