#pragma once

#include <vector>
#include <Person.h>
using namespace std;

enum class Status {
    UNMATCHED,
    ALLOTED
};

class Student : public Person{
    private: 
        float cgpa;
        std::vector<int> preferenceList;
        Status status;
    
    public:
        Student(int id, const string& name, float cgpa);

        void addPreference(int electiveId);

        void displayProfile() override;

        bool operator<(const Student& other) const;
        bool operator==(const Student& other) const;

        float getCgpa() const;
        const vector<int>& getPreferenceList() const;
        Status getStatus() const;

        void setStatus(Status newStatus);
};