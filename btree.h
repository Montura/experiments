#pragma once

#include <vector>
#include <cassert>
#include <mutex>

#include "entry.h"

///** Common Interface */
//template <typename Key, typename Value, typename Oit>
//struct IKeyValueStorage {
//    virtual bool insert(const Key& key, const Value& value) = 0;
//    virtual Oit get(const Key& key) = 0;
//    virtual bool remove(const Key& key) = 0;
//
//    virtual ~IKeyValueStorage() { };
//};

template <typename K, typename V>
class IOManager;

/** Tree */
template <typename K, typename V>
class BTree final {
    struct BTreeNode;
public:
    using Node = BTreeNode;
    using EntryT = Entry<K,V>;

    BTree(const std::string& path, int order);
    ~BTree();

    bool exist(const K &key);
    void set(const K &key, const V& value);
    V get(const K& key);
    bool remove(const K& key);

private:
    void insert(const K& key, const V& value);
    void traverse();

    BTreeNode* root = nullptr;
    const int t;

    IOManager<K,V> io_manager;
    using IOManagerT = IOManager<K,V>;

    /** Node */
    struct BTreeNode final {
        int used_keys;
        int t;
        char flag;
        int m_pos;
        std::vector<int> key_pos;
        std::vector<int> child_pos;

    public:
        BTreeNode(const int& t, bool isLeaf);

        bool is_leaf() const;
        bool is_full() const;
        char is_deleted_or_is_leaf() const;

        inline int max_key_num() const { return std::max(2 * t - 1, 0); }
        inline int max_child_num() const { return 2 * t; }

        int find_key_bin_search(IOManagerT& io_manager, const K& key);
        void split_child(IOManagerT& manager, const int idx, Node& curr_node);

        K get_key(IOManagerT& io_manager, const int idx);
        EntryT get_entry(IOManagerT& io_manager, const int idx);
        EntryT find(IOManagerT& io_manager, const K& key);

        void insert_non_full(IOManagerT& io_manager, const K& key, const V& value);
        void traverse(IOManagerT& io_manager);
        bool set(IOManagerT& io_manager, const K &key, const V &value);
        bool remove(IOManagerT& io_manager, const K& key);
    private:
        Node get_child(IOManagerT& io_manager, const int idx);

        bool remove_from_leaf(IOManagerT& io_manager, const int idx);
        bool remove_from_non_leaf(IOManagerT& io_manager, const int idx);

        int get_prev_entry_pos(IOManagerT& io_manager, const int idx);
        int get_next_entry_pos(IOManagerT& io_manager, const int idx);

        void merge_node(IOManagerT& io_manager, const int idx);
        void fill_node(IOManagerT& io_manager, const int idx);

        void borrow_from_prev_node(IOManagerT& io_manager, const int idx);
        void borrow_from_next_node(IOManagerT& io_manager, const int idx);
    };
};

#include "io_manager_impl.h"
#include "btree_impl.h"
#include "btree_node_impl.h"