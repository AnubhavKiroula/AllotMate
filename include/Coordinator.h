#pragma once

#include <string>
#include "Person.h"
using namespace std;

class Elective;

class Coordinator : public Person{

    private: 
        string department;
    
    public: 
        Coordinator(int id, const string& name, const string& department);

        void approveAllocation(Elective& elective);

        void displayProfile() override;

        const string& getDepartment() const;
};