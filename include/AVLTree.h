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
    int count = 0;

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

    // Used by remove: fixes imbalance on the way back up
    Node* rebalance(Node* n) {
        update(n);
        int bf = balanceFactor(n);
        if (bf > 1) {
            if (balanceFactor(n->left) < 0) n->left = rotateLeft(n->left);   // LR
            return rotateRight(n);                                           // LL
        }
        if (bf < -1) {
            if (balanceFactor(n->right) > 0) n->right = rotateRight(n->right); // RL
            return rotateLeft(n);                                              // RR
        }
        return n;
    }

    Node* insert(Node* n, const T& v) {
        if (!n) return new Node(v);
        if (v < n->data) n->left = insert(n->left, v);
        else n->right = insert(n->right, v);
        return rebalance(n);
    }

    Node* remove(Node* n, const T& v, bool& removed) {
        if (!n) return nullptr;
        if (v < n->data) {
            n->left = remove(n->left, v, removed);
        } else if (n->data < v) {
            n->right = remove(n->right, v, removed);
        } else {
            removed = true;
            if (!n->left || !n->right) {              // 0 or 1 child
                Node* child = n->left ? n->left : n->right;
                delete n;
                return child;
            }
            Node* m = n->right;                        // 2 children: use inorder successor
            while (m->left) m = m->left;
            n->data = m->data;
            bool ignored = false;
            n->right = remove(n->right, m->data, ignored);
        }
        return rebalance(n);
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
    AVLTree(const AVLTree&) = delete;             // Rule of 5 comes later
    AVLTree& operator=(const AVLTree&) = delete;

    void insert(const T& v) { root = insert(root, v); ++count; }

    bool remove(const T& v) {
        bool removed = false;
        root = remove(root, v, removed);
        if (removed) --count;
        return removed;
    }

    // Takes out the highest-priority element. Returns false if the tree is empty.
    bool extractMax(T& out) {
        if (!root) return false;
        Node* n = root;
        while (n->right) n = n->right;
        out = n->data;
        remove(out);
        return true;
    }

    bool empty() const { return root == nullptr; }
    int size() const { return count; }
    int treeHeight() const { return height(root); }

    template <typename F>
    void inorder(F visit) const { inorder(root, visit); }
};