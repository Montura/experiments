#pragma once

template <typename K, typename V>
BTree<K, V>::BTree(const std::string& path, int order) : t(order), io_manager(path, t) {
//    pthread_rwlock_init(&(rwLock), NULL);
    if (!io_manager.is_ready())
        return;

    int root_pos = io_manager.read_header();
    if (root_pos == IOManager<K,V>::INVALID_ROOT_POS)
        return;

    root = new Node(t, false);
    io_manager.read_node(root, root_pos);
}

template <typename K, typename V>
BTree<K, V>::~BTree() {
    delete root;
//    pthread_rwlock_destroy(&(rwLock));
}

template <typename K, typename V>
void BTree<K, V>::insert(const K& key, const V& value) {
    if (root == nullptr) {
        // write header
        int root_pos = io_manager.write_header();

        root = new Node(t, true);
        root->m_pos = root_pos;
        root->used_keys++;
        int node_pos = root->m_pos + io_manager.get_node_size_in_bytes(*root);
        root->key_pos[0] = node_pos;

        // write node root and key|value
        io_manager.write_node(*root, root->m_pos);
        io_manager.write_entry( { key, value }, node_pos);
    } else {
        if (root->is_full()) {
            Node newRoot(t, false);
            newRoot.child_pos[0] = root->m_pos;

            // Write node
            int posFile = io_manager.get_file_pos_end();
            newRoot.m_pos = posFile;
            io_manager.write_node(newRoot, newRoot.m_pos);

            newRoot.split_child(io_manager, 0, *root);

            // Find the child have new key
            int i = 0;
            K root_key = newRoot.get_entry(io_manager, 0).key;
            if (root_key < key)
                i++;

            // Read node
            int pos = newRoot.child_pos[i];
            Node node = io_manager.read_node(pos);
            node.insert_non_full(io_manager, key, value);

            io_manager.read_node(root, newRoot.m_pos);
            io_manager.writeUpdatePosRoot(newRoot.m_pos);
        } else {
            root->insert_non_full(io_manager, key, value);
        }
    }
}

template <typename K, typename V>
void BTree<K, V>::set(const K& key, const V& value) {
//    pthread_rwlock_wrlock(&(rwLock));

    if (!root || !root->set(io_manager, key, value))
        insert(key, value);

//    pthread_rwlock_unlock(&(rwLock));
}

template <typename K, typename V>
V BTree<K, V>::get(const K& key) {
    EntryT res = root ? root->find(io_manager, key) : EntryT {};
    return res.value;
}

template <typename K, typename V>
bool BTree<K, V>::exist(const K& key) {
//    pthread_rwlock_wrlock(&(rwLock));

    bool success = root && !root->find(io_manager, key).is_dummy();

//    pthread_rwlock_unlock(&(rwLock));
    return success;
}

template <typename K, typename V>
bool BTree<K, V>::remove(const K& key) {
//    pthread_rwlock_wrlock(&(rwLock));

    bool success = root && root->remove(io_manager, key);

    if (success && root->used_keys == 0) {
        if (root->is_leaf()) {
            io_manager.write_flag(root->is_deleted_or_is_leaf(), root->m_pos);
            delete root;
            root = nullptr;
//            io_manager.writeUpdatePosRoot(-1);
        } else {
            int pos = root->child_pos[0];
            io_manager.writeUpdatePosRoot(pos);
            io_manager.read_node(root, pos);
        }
    }

//    pthread_rwlock_unlock(&(rwLock));
    return success;
}

template <typename K, typename V>
void BTree<K, V>::traverse() {
    if (!root)
        root->traverse(io_manager);
}
