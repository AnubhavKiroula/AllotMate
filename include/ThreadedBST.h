#pragma once
#include <iostream>

// Right-threaded BST: a null right pointer is replaced by a "thread"
// to the inorder successor, so traversal needs no stack or recursion.
template <typename T>
class ThreadedBST {
    struct Node {
        T data;
        Node* left = nullptr;
        Node* right = nullptr;
        bool rightIsThread = false;   // true: right points to inorder successor
        explicit Node(const T& d) : data(d) {}
    };

    Node* root = nullptr;
    int count = 0;

    Node* leftmost(Node* n) const {
        while (n && n->left) n = n->left;
        return n;
    }

public:
    ThreadedBST() = default;

    ~ThreadedBST() {
        Node* cur = leftmost(root);
        while (cur) {
            Node* next = cur->rightIsThread ? cur->right : leftmost(cur->right);
            delete cur;
            cur = next;
        }
    }
    ThreadedBST(const ThreadedBST&) = delete;           // Rule of 5 later
    ThreadedBST& operator=(const ThreadedBST&) = delete;

    void insert(const T& v) {
        Node* node = new Node(v);
        ++count;
        if (!root) { root = node; return; }

        Node* cur = root;
        while (true) {
            if (v < cur->data) {
                if (!cur->left) {
                    cur->left = node;
                    node->right = cur;            // successor of a new left child = parent
                    node->rightIsThread = true;
                    return;
                }
                cur = cur->left;
            } else {
                if (cur->rightIsThread || !cur->right) {
                    node->right = cur->right;     // inherit cur's old successor (or null)
                    node->rightIsThread = (cur->right != nullptr);
                    cur->right = node;
                    cur->rightIsThread = false;
                    return;
                }
                cur = cur->right;
            }
        }
    }

    // Inorder traversal: no stack, no recursion
    template <typename F>
    void inorder(F visit) const {
        Node* cur = leftmost(root);
        while (cur) {
            visit(cur->data);
            cur = cur->rightIsThread ? cur->right : leftmost(cur->right);
        }
    }

    bool empty() const { return root == nullptr; }
    int size() const { return count; }
};