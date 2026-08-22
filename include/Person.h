#pragma once

#include <string>
using namespace std;

class Person {

    protected:
        int id;
        string name;

    public:
        virtual void displayProfile() = 0;
        virtual ~Person() = default;
        
};