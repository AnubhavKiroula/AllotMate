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

    // Forward declarations of nested exception types
    class SeatOverflowException;
    class InvalidPreferenceException;
    class AlreadyAllottedException;
    class DatabaseException;
};

// Sub-exception definitions outside class body (where AllocationException is a complete type)
class AllocationException::SeatOverflowException : public AllocationException {
public:
    explicit SeatOverflowException(const std::string& msg = "Seat capacity overflow for elective")
        : AllocationException(msg) {}
};

class AllocationException::InvalidPreferenceException : public AllocationException {
public:
    explicit InvalidPreferenceException(const std::string& msg = "Invalid or duplicate elective preference ranking")
        : AllocationException(msg) {}
};

class AllocationException::AlreadyAllottedException : public AllocationException {
public:
    explicit AlreadyAllottedException(const std::string& msg = "Student is already allotted an elective")
        : AllocationException(msg) {}
};

class AllocationException::DatabaseException : public AllocationException {
public:
    explicit DatabaseException(const std::string& msg = "Database operation error")
        : AllocationException(msg) {}
};