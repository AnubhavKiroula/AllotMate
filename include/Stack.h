#pragma once

#include <vector>
#include <stdexcept>

template <typename T>
class Stack {

    private: 
        std::vector<T> elements;

    public:
        Stack() = default;
        ~Stack() = default;

        void push(const T& value);
        void pop();

        const T& top() const;

        bool empty() const;
        int size() const;
};

template <typename T>
void Stack<T>::push(const T& value){
    elements.push_back(value);
}

template <typename T>
void Stack<T>::pop(){

    if(elements.empty()){
        throw std::out_of_range(
            "Cannot pop from an empty Stack."
        );
    }
    elements.pop_back();
}

template <typename T>
const T& Stack<T>::top() const {

    if(elements.empty()){
        throw std::out_of_range(
            "Cannot access top of an empty Stack."
        );
    }
    return elements.back();
}

template <typename T>
bool Stack<T>::empty() const{
    return elements.empty();
} 

template <typename T>
int Stack<T>::size() const{
    return static_cast<int>(elements.size());
}