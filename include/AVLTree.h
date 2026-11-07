#pragma once
#include <algorithm>
#include <iostream>

template <typename T>
class AVLTree {
    struct Node {
        T data;
        Node* left = nullptr;
        Node* right = nullptr;
        int height = 1;
        explicit Node(const T& d) : data(d) {}
    };

    Node* root = nullptr;

    static int height(Node* n) { return n ? n->height : 0; }
    static int balanceFactor(Node* n) { return n ? height(n->left) - height(n->right) : 0; }
    static void update(Node* n) { n->height = 1 + std::max(height(n->left), height(n->right)); }

    Node* rotateRight(Node* y) {
        Node* x = y->left;
        Node* t = x->right;
        x->right = y;
        y->left = t;
        update(y);
        update(x);
        return x;
    }

    Node* rotateLeft(Node* x) {
        Node* y = x->right;
        Node* t = y->left;
        y->left = x;
        x->right = t;
        update(x);
        update(y);
        return y;
    }

    Node* insert(Node* n, const T& v) {
        if (!n) return new Node(v);
        if (v < n->data) n->left = insert(n->left, v);
        else n->right = insert(n->right, v);

        update(n);
        int bf = balanceFactor(n);

        if (bf > 1 && v < n->left->data) return rotateRight(n);            // LL
        if (bf < -1 && !(v < n->right->data)) return rotateLeft(n);        // RR
        if (bf > 1 && !(v < n->left->data)) {                              // LR
            n->left = rotateLeft(n->left);
            return rotateRight(n);
        }
        if (bf < -1 && v < n->right->data) {                               // RL
            n->right = rotateRight(n->right);
            return rotateLeft(n);
        }
        return n;
    }

    void destroy(Node* n) {
        if (!n) return;
        destroy(n->left);
        destroy(n->right);
        delete n;
    }

    template <typename F>
    void inorder(Node* n, F visit) const {
        if (!n) return;
        inorder(n->left, visit);
        visit(n->data);
        inorder(n->right, visit);
    }

public:
    AVLTree() = default;
    ~AVLTree() { destroy(root); }
    AVLTree(const AVLTree&) = delete;             // copy/move added later (Rule of 5)
    AVLTree& operator=(const AVLTree&) = delete;

    void insert(const T& v) { root = insert(root, v); }
    bool empty() const { return root == nullptr; }
    int treeHeight() const { return height(root); }

    template <typename F>
    void inorder(F visit) const { inorder(root, visit); }
};