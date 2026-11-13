#pragma once
#include <iostream>
#include <utility>
#include <vector>

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

    // Frees all nodes using the threaded walk (no recursion)
    void clear() {
        Node* cur = leftmost(root);
        while (cur) {
            Node* next = cur->rightIsThread ? cur->right : leftmost(cur->right);
            delete cur;
            cur = next;
        }
        root = nullptr;
        count = 0;
    }

    // Inserts the middle element first, so the copy stays balanced
    void insertBalanced(const std::vector<T>& v, int lo, int hi) {
        if (lo > hi) return;
        int mid = lo + (hi - lo) / 2;
        insert(v[mid]);
        insertBalanced(v, lo, mid - 1);
        insertBalanced(v, mid + 1, hi);
    }

public:
    ThreadedBST() = default;
    ~ThreadedBST() { clear(); }

    // Copy: collect inorder, then re-insert so the threads are rebuilt correctly
    ThreadedBST(const ThreadedBST& other) {
        std::vector<T> items;
        other.inorder([&](const T& x) { items.push_back(x); });
        insertBalanced(items, 0, static_cast<int>(items.size()) - 1);
    }

    // Copy assignment: copy-and-swap
    ThreadedBST& operator=(const ThreadedBST& other) {
        if (this != &other) {
            ThreadedBST temp(other);
            std::swap(root, temp.root);
            std::swap(count, temp.count);
        }
        return *this;
    }

    // Move: steal the nodes, leave the source empty
    ThreadedBST(ThreadedBST&& other) noexcept : root(other.root), count(other.count) {
        other.root = nullptr;
        other.count = 0;
    }

    ThreadedBST& operator=(ThreadedBST&& other) noexcept {
        if (this != &other) {
            clear();
            root = other.root;
            count = other.count;
            other.root = nullptr;
            other.count = 0;
        }
        return *this;
    }

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