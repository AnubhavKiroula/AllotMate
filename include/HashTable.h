#pragma once

#include <vector>

class HashTable{
    private:
        struct Entry{
            int key;
            int value;
            bool occupied;

            Entry();
        };

        std::vector<Entry> table;
        int capacity;
        int currentSize;

        int hash(int key) const;

    public:
        explicit HashTable(int capacity = 101);
        ~HashTable() = default;

        void insert(int key, int value);

        int get(int key) const;

        bool contains(int key) const;

        int size() const;
};