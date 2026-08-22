#pragma once

#include <exception>
using namespace std;

class AllocationException : public exception {

    public:
        class SeatOverflowException;
        class InvalidPreferenceException;
        class AlreadyAllottedException;

        virtual ~AllocationException() = default;
};