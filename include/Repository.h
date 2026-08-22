#pragma once 

#include <vector>
using namespace std;

template <typename T>
class Repository{

    public:
        virtual void save(T entity) = 0;
        virtual T findById(int id) = 0;
        virtual vector<T> findAll() = 0;

        virtual ~Repository() = default;
};

