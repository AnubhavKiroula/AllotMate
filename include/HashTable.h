#pragma once

#include <vector>
#include <memory>
#include <stdexcept>
#include <utility>

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

    // Grows the table to about twice its size and re-inserts every entry.
    // Called by insert() when the load factor would pass 0.7.
    void rehash();

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
void HashTable<K, V>::rehash() {

    // Keep the old slots and switch to a bigger, empty table.
    std::vector<Entry> oldTable = std::move(table);
    capacity = capacity * 2 + 1;
    table = std::vector<Entry>(capacity);
    currentSize = 0;

    // Every key must be re-hashed, because hash() depends on capacity.
    // The value pointers are MOVED, not copied, so a value never changes its
    // address -- references returned earlier by find() stay valid.
    // Deleted (tombstone) slots are simply not carried over.
    for (Entry& old : oldTable) {
        if (!old.occupied) {
            continue;
        }

        int index = hash(old.key);
        while (table[index].occupied) {
            index = (index + 1) % capacity; // linear probing
        }

        table[index].key = old.key;
        table[index].value = std::move(old.value);
        table[index].occupied = true;
        ++currentSize;
    }
}


template <typename K, typename V>
void HashTable<K, V>::insert(
    const K& key,
    const V& value
) {

    // Grow before adding a NEW key if the table would become more than 70%
    // full (load factor 0.7). Above that, linear probing chains get long and
    // a fixed-size table eventually runs out of slots. Integer maths avoids
    // floating point: (size + 1) / capacity > 0.7  <=>  (size + 1) * 10 > capacity * 7.
    if (!contains(key) && (currentSize + 1) * 10 > capacity * 7) {
        rehash();
    }

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

