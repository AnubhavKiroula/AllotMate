#include "AVLTree.h"
#include <iostream>

int main() {
    AVLTree<int> t;
    for (int i = 1; i <= 7; i++) t.insert(i);   // sorted input is the worst case for a plain BST

    std::cout << "Inorder: ";
    t.inorder([](int x) { std::cout << x << " "; });
    std::cout << "\nHeight: " << t.treeHeight() << std::endl;
    return 0;
}