#pragma once

namespace btree {
    template<typename K, typename V>
    BTree<K, V>::BTree(const std::string &path, const int16_t order) : t(order), io_manager(path, t), root() {
//    pthread_rwlock_init(&(rwLock), NULL);
        if (!io_manager.is_ready())
            return;

        auto root_pos = io_manager.read_header();
        if (root_pos == IOManager<K, V>::INVALID_ROOT_POS)
            return;

        root = io_manager.read_node(root_pos);
    }

    template<typename K, typename V>
    BTree<K, V>::~BTree() {
//    pthread_rwlock_destroy(&(rwLock));
    }

    template<typename K, typename V>
    void BTree<K, V>::set(const K &key, const V &value) {
//    pthread_rwlock_wrlock(&(rwLock));

        EntryT e {key, value};
        if (!root.is_valid() || !root.set(io_manager, e))
            insert(e);

//    pthread_rwlock_unlock(&(rwLock));
    }

    template<typename K, typename V>
    void BTree<K, V>::set(const K &key, const V& value, int32_t size) {
//    pthread_rwlock_wrlock(&(rwLock));

        EntryT e {key, value, size};
        if (!root.is_valid() || !root.set(io_manager, e))
            insert(e);

//    pthread_rwlock_unlock(&(rwLock));
    }

    template<typename K, typename V>
    std::optional <V> BTree<K, V>::get(const K &key) {
        EntryT res = root.is_valid() ? root.find(io_manager, key) : EntryT{};
        return res.value();
    }

    template<typename K, typename V>
    bool BTree<K, V>::exist(const K &key) {
//    pthread_rwlock_wrlock(&(rwLock));

        bool success = root.is_valid() && root.find(io_manager, key).is_valid();

//    pthread_rwlock_unlock(&(rwLock));
        return success;
    }

    template<typename K, typename V>
    bool BTree<K, V>::remove(const K &key) {
//    pthread_rwlock_wrlock(&(rwLock));

        bool success = root.is_valid() && root.remove(io_manager, key);

        if (success && root.used_keys == 0) {
            if (root.is_leaf) {
                root = Node();
                io_manager.write_invalidated_root();
            } else {
                auto pos = root.child_pos[0];
                io_manager.write_new_pos_for_root_node(pos);
                root = io_manager.read_node(pos);
            }
        }

//    pthread_rwlock_unlock(&(rwLock));
        return success;
    }

    template<typename K, typename V>
    void BTree<K, V>::insert(const EntryT& e) {
        if (!root.is_valid()) {
            // write header
            auto root_pos = io_manager.write_header();

            root = Node(t, true);
            root.m_pos = root_pos;
            root.used_keys++;

            auto entry_pos = root.m_pos + root.get_node_size_in_bytes();
            root.key_pos[0] = entry_pos;

            // write node root and key|value
            io_manager.write_node(root, root.m_pos);
            io_manager.write_entry(e, entry_pos);
        } else {
            if (root.is_full()) {
                Node newRoot(t, false);
                newRoot.child_pos[0] = root.m_pos;

                // Write node
                newRoot.m_pos = io_manager.get_file_pos_end();
                io_manager.write_node(newRoot, newRoot.m_pos);

                newRoot.split_child(io_manager, 0, root);

                // Find the child have new key
                int32_t i = 0;
                K root_key = newRoot.get_key(io_manager, 0);
                if (root_key < e.key)
                    i++;

                // Read node
                auto pos = newRoot.child_pos[i];
                Node node = io_manager.read_node(pos);
                node.insert_non_full(io_manager, e);

                root = io_manager.read_node(newRoot.m_pos);
                io_manager.write_new_pos_for_root_node(newRoot.m_pos);
            } else {
                root.insert_non_full(io_manager, e);
            }
        }
    }
}