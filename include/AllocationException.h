#pragma once

#include <exception>
#include <string>

class AllocationException : public std::exception {
protected:
    std::string message;

public:
    explicit AllocationException(const std::string& msg) : message(msg) {}
    virtual const char* what() const noexcept override {
        return message.c_str();
    }
    virtual ~AllocationException() = default;

    // Sub-exception 1: Thrown when an elective has exceeded its seat capacity
    class SeatOverflowException : public AllocationException {
    public:
        explicit SeatOverflowException(const std::string& msg = "Seat capacity overflow for elective")
            : AllocationException(msg) {}
    };

    // Sub-exception 2: Thrown when a student submits duplicate/invalid elective preferences
    class InvalidPreferenceException : public AllocationException {
    public:
        explicit InvalidPreferenceException(const std::string& msg = "Invalid or duplicate elective preference ranking")
            : AllocationException(msg) {}
    };

    // Sub-exception 3: Thrown when a student is already allotted an elective in a round
    class AlreadyAllottedException : public AllocationException {
    public:
        explicit AlreadyAllottedException(const std::string& msg = "Student is already allotted an elective")
            : AllocationException(msg) {}
    };

    // Sub-exception 4: Thrown when database connection or query execution fails
    class DatabaseException : public AllocationException {
    public:
        explicit DatabaseException(const std::string& msg = "Database operation error")
            : AllocationException(msg) {}
    };
};