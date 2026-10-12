#pragma once

#include <string>
using namespace std;

class Person {

    protected:
        int id;
        string name;

    public:
        int getId() const{
            return id;
        }
        virtual void displayProfile() = 0;
        virtual ~Person() = default;
        
};