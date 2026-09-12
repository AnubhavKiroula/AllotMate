#pragma once

#include <vector>
#include <stdexcept>
#include <utility>

template <typename T>
class PriorityQueue {
    private:
        std::vector<T> heap;

        void heapifyUp(int index);
        void heapifyDown(int index);
    
    public: 
        PriorityQueue() = default;
        ~PriorityQueue() = default;
        
        void push(const T& value);
        void pop();

        const T& top() const;

        bool empty() const;
        int size() const;
};

template <typename T>
void PriorityQueue<T>::heapifyUp(int index){

    while(index > 0){
        int parent = (index -1)/2;

        if(heap[parent] < heap[index]){
            std::swap(heap[parent], heap[index]);
            index = parent;
        }
        else {
            break;
        }
    }
}

template <typename T>
void PriorityQueue<T>::heapifyDown(int index){
    
    int heapSize = static_cast<int>(heap.size());

    while (true){

        int left = 2 * index + 1;
        int right = 2 * index + 2;
        int largest = index;

        if(left < heapSize &&
            heap[largest] < heap[left]){
                largest = left;
        } 

        if(right < heapSize &&
            heap[largest] < heap[right]){
                largest = right;
        }
        
        if(largest == index){
            break;
        }

        std::swap(heap[index], heap[largest]);
        index = largest;
    }
}

template <typename T>
void PriorityQueue<T>::push(const T& value){
    heap.push_back(value);

    heapifyUp(static_cast<int>(heap.size())-1);
}

template <typename T>
void PriorityQueue<T>::pop(){

    if(heap.empty()) {
        throw std::out_of_range(
            "Cannot pop from an empty PriorityQueue."
        );
    }

    heap[0] = heap.back();
    heap.pop_back();

    if(!heap.empty()){
        heapifyDown(0);
    }
}

template <typename T>
const T& PriorityQueue<T>::top() const {

    if(heap.empty()) {
        throw std::out_of_range(
            "Cannot access top of an empty PriorityQueue."
        );
    }
    return heap[0];
}
template <typename T>
bool PriorityQueue<T>::empty() const {
    return heap.empty();
}

template <typename T>
int PriorityQueue<T>::size() const {
    return static_cast<int>(heap.size());
}