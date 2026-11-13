#include "AVLTree.h"
#include <iostream>

int main() {
    AVLTree<int> t;
    for (int i = 1; i <= 7; i++) t.insert(i);
    std::cout << "Height after 1..7: " << t.treeHeight() << " (expect 3)\n";

    t.remove(4);                      // remove the root
    std::cout << "After remove(4): ";
    t.inorder([](int x) { std::cout << x << " "; });
    std::cout << "\nSize: " << t.size() << " (expect 6)\n";

    int top = 0;
    t.extractMax(top);
    std::cout << "extractMax: " << top << " (expect 7)\n";
    std::cout << "Size: " << t.size() << " (expect 5)\n";

    std::cout << "remove(99) on missing value: " << t.remove(99) << " (expect 0)\n";
    return 0;
}