#pragma once

#include <vector>
#include <memory>
#include <stdexcept>

template <typename K, typename V>
class HashTable {
private:
    struct Entry {
        K key{};
        std::unique_ptr<V> value;
        bool occupied = false ;
        bool deleted = false;
    };

    std::vector<Entry> table;
    int capacity;
    int currentSize;

    int hash(const K& key) const;

public:
    explicit HashTable(int capacity = 101);

    void insert(const K& key, const V& value);
    V& find(const K& key);
    const V& find(const K& key) const;
    bool contains(const K& key) const;
    void remove(const K& key);

    int size() const;
};


template <typename K, typename V>
HashTable<K, V>::HashTable(int capacity)
    : table(capacity),
      capacity(capacity),
      currentSize(0) {

        if(capacity <= 0){
            throw std::invalid_argument( 
                "HashTable capacity must be positive."
            );
        }
}

template <typename K, typename V>
int HashTable<K, V>::hash(const K& key) const {

    return (key % capacity + capacity) % capacity;
}


template <typename K, typename V>
void HashTable<K, V>::insert(
    const K& key,
    const V& value
) {

    int index = hash(key);
    int firstDeleted = -1;

    for (int i = 0; i < capacity; ++i) {

        int probeIndex = (index + i) % capacity;
        Entry& entry = table[probeIndex];

        if(entry.occupied && entry.key == key){
            entry.value = std::make_unique<V>(value);
            return;
        }

        if(entry.deleted && firstDeleted == -1){
            firstDeleted = probeIndex;
        }

        if( !entry.occupied && !entry.deleted){
            int target = (firstDeleted != -1)
                ? firstDeleted
                : probeIndex;
            
            Entry& destination = table[target];
            destination.key = key;
            destination.value = std::make_unique<V>(value);
            destination.occupied = true;
            destination.deleted = false;

            ++currentSize;
            return;
        }
    }

    if(firstDeleted != -1){
        Entry& destination = table[firstDeleted];
        destination.key = key;
        destination.value = std::make_unique<V>(value);
        destination.occupied = true;
        destination.deleted = false;

        ++currentSize;
        return;
    }

    throw std::overflow_error(
        "HashTable is full."
    );
}


template <typename K, typename V>
V& HashTable<K, V>::find(const K& key) {

    int index = hash(key);

    for (int i = 0; i < capacity; ++i) {

        int probeIndex = (index + i) % capacity;
        Entry& entry = table[probeIndex];

        if(!entry.occupied && !entry.deleted){
            break;
        }

        if(entry.occupied && entry.key == key){
            return *entry.value;
        }
    }

    throw std::out_of_range(
        "Key not found in HashTable."
    );
}

template <typename K, typename V>
const V& HashTable<K, V>::find(const K& key) const{
    int index = hash(key);

    for(int i=0; i< capacity; ++i){
        int probeIndex = (index + i)% capacity;
        const Entry& entry = table[probeIndex];

        if(!entry.occupied && !entry.deleted){
            break;
        }

        if(entry.occupied && entry.key == key){
            return *entry.value;
        }
    }

    throw std::out_of_range("Key not found in HashTable.");
}

template <typename K, typename V>
bool HashTable<K, V>::contains(const K& key) const {

    int index = hash(key);

    for (int i = 0; i < capacity; ++i) {

        int probeIndex = (index + i) % capacity;
        const Entry& entry = table[probeIndex];

        if (!entry.occupied && !entry.deleted) {
            return false;
        }

        if (entry.occupied && entry.key == key) {
            return true;
        }
    }

    return false;
}


template <typename K, typename V>
void HashTable<K, V>::remove(const K& key) {

    int index = hash(key);

    for (int i = 0; i < capacity; ++i) {

        int probeIndex = (index + i) % capacity;
        Entry& entry = table[probeIndex];

        if (!entry.occupied && !entry.deleted) {
            return;
        }

        if (entry.occupied && entry.key == key) {

            entry.value.reset();
            entry.occupied = false;
            entry.deleted = true;
            --currentSize;
            return;
        }
    }
}


template <typename K, typename V>
int HashTable<K, V>::size() const {

    return currentSize;
}

