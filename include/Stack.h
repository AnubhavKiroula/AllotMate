
#pragma once

#include <stdexcept>
#include <new>
#include <utility>

template <typename T>
class Stack {
private:
    T* elements;
    int capacity;
    int currentSize;

    void resize() {
        // A moved-from stack has capacity 0, and 0 * 2 is still 0.
        int newCapacity = (capacity == 0) ? 10 : capacity * 2;

        T* newElements = static_cast<T*>(
            ::operator new(sizeof(T) * newCapacity)
        );

        int constructed = 0;

        try {
            for (; constructed < currentSize; ++constructed) {
                new (&newElements[constructed])
                    T(std::move_if_noexcept(elements[constructed]));
            }
        } catch (...) {
            for (int i = 0; i < constructed; ++i) {
                newElements[i].~T();
            }
            ::operator delete(newElements);
            throw;
        }

        for (int i = 0; i < currentSize; ++i) {
            elements[i].~T();
        }

        ::operator delete(elements);

        elements = newElements;
        capacity = newCapacity;
    }

public:
    explicit Stack(int initialCapacity = 10)
        : elements(nullptr),
          capacity(initialCapacity),
          currentSize(0) {

        if (initialCapacity <= 0) {
            throw std::invalid_argument(
                "Stack capacity must be positive."
            );
        }

        elements = static_cast<T*>(
            ::operator new(sizeof(T) * capacity)
        );
    }

    ~Stack() {
        for (int i = 0; i < currentSize; ++i) {
            elements[i].~T();
        }

        ::operator delete(elements);
    }

    Stack(const Stack& other)
        : elements(static_cast<T*>(
              ::operator new(sizeof(T) * other.capacity))),
          capacity(other.capacity),
          currentSize(0) {

        try {
            for (; currentSize < other.currentSize; ++currentSize) {
                new (&elements[currentSize]) T(
                    other.elements[currentSize]
                );
            }
        } catch (...) {
            for (int i = 0; i < currentSize; ++i) {
                elements[i].~T();
            }
            ::operator delete(elements);
            throw;
        }
    }

    Stack& operator=(const Stack& other) {
        if (this == &other) {
            return *this;
        }

        Stack temporary(other);
        swap(temporary);
        return *this;
    }

    Stack(Stack&& other) noexcept
        : elements(other.elements),
          capacity(other.capacity),
          currentSize(other.currentSize) {
        other.elements = nullptr;
        other.capacity = 0;
        other.currentSize = 0;
    }

    Stack& operator=(Stack&& other) noexcept {
        if (this != &other) {
            for (int i = 0; i < currentSize; ++i) {
                elements[i].~T();
            }

            ::operator delete(elements);

            elements = other.elements;
            capacity = other.capacity;
            currentSize = other.currentSize;

            other.elements = nullptr;
            other.capacity = 0;
            other.currentSize = 0;
        }

        return *this;
    }

    void swap(Stack& other) noexcept {
        using std::swap;
        swap(elements, other.elements);
        swap(capacity, other.capacity);
        swap(currentSize, other.currentSize);
    }

    void push(const T& value) {
        if (currentSize == capacity) {
            resize();
        }

        new (&elements[currentSize]) T(value);
        ++currentSize;
    }

    void pop() {
        if (empty()) {
            throw std::out_of_range(
                "Cannot pop from an empty Stack."
            );
        }

        --currentSize;
        elements[currentSize].~T();
    }

    const T& top() const {
        if (empty()) {
            throw std::out_of_range(
                "Cannot access top of an empty Stack."
            );
        }

        return elements[currentSize - 1];
    }

    bool empty() const {
        return currentSize == 0;
    }

    int size() const {
        return currentSize;
    }
};
