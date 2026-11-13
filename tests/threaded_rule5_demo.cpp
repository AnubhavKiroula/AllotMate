#include "ThreadedBST.h"
#include <iostream>

int main() {
    ThreadedBST<int> a;
    for (int v : {50, 30, 70, 20, 40}) a.insert(v);

    ThreadedBST<int> b = a;                  // copy constructor
    b.insert(99);
    std::cout << "a size: " << a.size() << " (expect 5)\n";
    std::cout << "b size: " << b.size() << " (expect 6)\n";
    b.inorder([](int x) { std::cout << x << " "; });
    std::cout << "(expect 20 30 40 50 70 99)\n";

    ThreadedBST<int> c = std::move(a);       // move constructor
    std::cout << "c size: " << c.size() << " (expect 5), a size: " << a.size() << " (expect 0)\n";

    ThreadedBST<int> d;
    d = c;                                   // copy assignment
    d.insert(1);
    std::cout << "d size: " << d.size() << " (expect 6), c size: " << c.size() << " (expect 5)\n";

    ThreadedBST<int> e;
    e = std::move(d);                        // move assignment
    std::cout << "e size: " << e.size() << " (expect 6), d size: " << d.size() << " (expect 0)\n";
    e.inorder([](int x) { std::cout << x << " "; });
    std::cout << "(expect 1 20 30 40 50 70)\n";
    return 0;
}