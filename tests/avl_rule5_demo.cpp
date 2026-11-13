#include "AVLTree.h"
#include <iostream>

int main() {
    AVLTree<int> a;
    for (int i = 1; i <= 5; i++) a.insert(i);

    AVLTree<int> b = a;          // copy constructor
    b.insert(99);
    std::cout << "a size: " << a.size() << " (expect 5)\n";
    std::cout << "b size: " << b.size() << " (expect 6)\n";

    AVLTree<int> c;
    c = a;                       // copy assignment
    c.remove(1);
    std::cout << "a size: " << a.size() << " (expect 5)\n";
    std::cout << "c size: " << c.size() << " (expect 4)\n";

    AVLTree<int> d = std::move(a);   // move constructor
    std::cout << "d size: " << d.size() << " (expect 5)\n";
    std::cout << "a size after move: " << a.size() << " (expect 0)\n";

    AVLTree<int> e;
    e = std::move(d);                // move assignment
    std::cout << "e size: " << e.size() << " (expect 5)\n";
    std::cout << "d size after move: " << d.size() << " (expect 0)\n";
    return 0;
}