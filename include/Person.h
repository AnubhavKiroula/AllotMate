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

        // Read-only accessors for the shared identity fields. Added in Phase 2
        // because the persistence layer (and the matching engine) need to read a
        // Person's id/name without being able to set them. Purely additive --
        // no existing behaviour changes.
        int getId() const { return id; }
        const string& getName() const { return name; }
};