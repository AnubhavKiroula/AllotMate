#include "ThreadedBST.h"
#include <iostream>

int main() {
    ThreadedBST<int> t;
    int vals[] = {50, 30, 70, 20, 40, 60, 80, 35};
    for (int v : vals) t.insert(v);

    std::cout << "Inorder: ";
    t.inorder([](int x) { std::cout << x << " "; });
    std::cout << "\nSize: " << t.size() << " (expect 8)\n";
    return 0;
}