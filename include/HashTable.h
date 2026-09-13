#pragma once

#include <vector>
#include <stdexcept>

template <typename K, typename V>
class HashTable {
private:
    struct Entry {
        K key;
        V value;
        bool occupied;

        Entry();
    };

    std::vector<Entry> table;
    int capacity;
    int currentSize;

    int hash(const K& key) const;

public:
    explicit HashTable(int capacity = 101);
    ~HashTable() = default;

    void insert(const K& key, const V& value);
    V find(const K& key) const;
    bool contains(const K& key) const;
    void remove(const K& key);

    int size() const;
};


template <typename K, typename V>
HashTable<K, V>::Entry::Entry()
    : key(),
      value(),
      occupied(false) {
}


template <typename K, typename V>
HashTable<K, V>::HashTable(int capacity)
    : table(capacity),
      capacity(capacity),
      currentSize(0) {

    if (capacity <= 0) {
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

    for (int i = 0; i < capacity; ++i) {

        int probeIndex = (index + i) % capacity;

        if (!table[probeIndex].occupied) {

            table[probeIndex].key = key;
            table[probeIndex].value = value;
            table[probeIndex].occupied = true;

            ++currentSize;
            return;
        }

        if (table[probeIndex].key == key) {

            table[probeIndex].value = value;
            return;
        }
    }

    throw std::overflow_error(
        "HashTable is full."
    );
}


template <typename K, typename V>
V HashTable<K, V>::find(const K& key) const {

    int index = hash(key);

    for (int i = 0; i < capacity; ++i) {

        int probeIndex = (index + i) % capacity;

        if (!table[probeIndex].occupied) {
            throw std::out_of_range(
                "Key not found in HashTable."
            );
        }

        if (table[probeIndex].key == key) {
            return table[probeIndex].value;
        }
    }

    throw std::out_of_range(
        "Key not found in HashTable."
    );
}


template <typename K, typename V>
bool HashTable<K, V>::contains(const K& key) const {

    int index = hash(key);

    for (int i = 0; i < capacity; ++i) {

        int probeIndex = (index + i) % capacity;

        if (!table[probeIndex].occupied) {
            return false;
        }

        if (table[probeIndex].key == key) {
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

        if (!table[probeIndex].occupied) {
            return;
        }

        if (table[probeIndex].key == key) {

            table[probeIndex].occupied = false;
            --currentSize;
            return;
        }
    }
}


template <typename K, typename V>
int HashTable<K, V>::size() const {

    return currentSize;
}