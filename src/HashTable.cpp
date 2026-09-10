#include "HashTable.h"

HashTable::Entry::Entry() 
    : key(0),
      value(0),
      occupied(false){

}

HashTable::HashTable(int capacity)
    : table(capacity),
      capacity(capacity),
      currentSize(0){

}

int HashTable::hash(int key) const{
    return (key % capacity + capacity) % capacity;
}

void HashTable::insert(int key, int value){

    int index = hash(key);

    for(int i=0; i<capacity; ++i){

        int probeIndex = (index + i) % capacity;

        if (!table[probeIndex].occupied){
            table[probeIndex].key = key;
            table[probeIndex].value = value;
            table[probeIndex].occupied = true;
            ++currentSize;
            return;
        }

        if (table[probeIndex].key == key){
            table[probeIndex].value = value;
            return;
        }
    }
}

int HashTable::get(int key) const {
    
    int index = hash(key);

    for(int i = 0 ; i < capacity ; ++i){

        int probeindex = (index+i) % capacity;

        if(!table[probeindex].occupied){
            return 0;
        }

        if(table[probeindex].key == key){
            return table[probeindex].value;
        }
    }
    return 0;
}

bool HashTable::contains(int key) const {

    int index = hash(key);

    for(int i=0; i<capacity; ++i){

        int probeIndex = (index + i) % capacity;

        if(!table[probeIndex].occupied){
            return false;
        }

        if(table[probeIndex].key == key){
            return true;
        }
    }
    return false;
}

int HashTable::size() const{
    return currentSize;
}