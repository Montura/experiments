#pragma once

#include "utils/utils.h"

namespace btree {
    template<typename K, typename V>
    int64_t IOManager<K, V>::write_header() {
        file.set_pos(0);

        file.write_next(t);
        file.write_next<uint8_t>(sizeof(K));
        file.write_next<uint8_t>(get_value_type_code<V>());
        file.write_next<uint8_t>(get_element_size<V>());
        file.write_next(INITIAL_ROOT_POS_IN_HEADER);
        return file.get_pos();
    }

    template<typename K, typename V>
    int64_t IOManager<K, V>::read_header() {
        file.set_pos(0);

        auto t_from_file = file.read_int16();
        validate(t == t_from_file,
                 "The sizeof(KEY) for your tree doesn't equal to the KEY used in storage: ");

        auto key_size = file.read_byte();
        validate(key_size == sizeof(K),
                 "The sizeof(KEY) for your tree doesn't equal to the KEY used in storage: ");

        auto value_type_code = file.read_byte();
        validate(value_type_code == get_value_type_code<V>(),
                 "The sizeof(KEY) for your tree doesn't equal to the KEY used in storage: ");

        auto element_size = file.read_byte();
        validate(element_size == get_element_size<V>(),
                 "The sizeof(KEY) for your tree doesn't equal to the KEY used in storage: ");

        auto posRoot = file.read_int32();
        return posRoot;
    }

    template<typename K, typename V>
    IOManager<K, V>::IOManager(const std::string &path, const int16_t user_t) : t(user_t), file(path, 0) {}

    template<typename K, typename V>
    bool IOManager<K, V>::is_ready() const {
        return !file.isEmpty();
    }

    template<typename K, typename V>
    void IOManager<K, V>::write_entry(const K &key, const V &value, const int64_t pos) {
        file.set_pos(pos);

        file.write_next(key);
        file.write_next(value);
    }

    template<typename K, typename V>
    Entry <K, V> IOManager<K, V>::read_entry(const int64_t pos) {
        file.set_pos(pos);

        K key = file.read_next<K>();
        auto[value, size] = file.read_next_data<V, typename EntryT::ValueType>();
        return {key, value, size};
    }

    template<typename K, typename V>
    K IOManager<K, V>::read_key(const int64_t pos) {
        file.set_pos(pos);

        return file.read_next<K>();
    }

    template<typename K, typename V>
    std::optional<V> IOManager<K, V>::read_value(const ValueType value, const int32_t size) {
        if constexpr (is_string_v<V>) {
            auto *casted_value = reinterpret_cast<const typename V::value_type *>(value);
            return V(casted_value, size);
        } else {
            return V(value);
        }
    }

    template<typename K, typename V>
    void IOManager<K, V>::write_flag(uint8_t flag, const int64_t pos) {
        file.set_pos(pos);

        file.write_next(flag);
    }

    template<typename K, typename V>
    void IOManager<K, V>::write_new_pos_for_root_node(const int64_t posRoot) {
        file.set_pos(ROOT_POS_IN_HEADER);

        file.write_next(posRoot);
    }

    template<typename K, typename V>
    void IOManager<K, V>::write_invalidated_root() {
        file.set_pos(ROOT_POS_IN_HEADER);

        file.write_next(INVALID_ROOT_POS);
        file.shrink_to_fit();
    }

    template<typename K, typename V>
    int64_t IOManager<K, V>::write_node(const Node& node, const int64_t pos) {
        file.set_pos(pos);

        file.write_next(node.is_leaf);
        file.write_next(node.used_keys);
        file.write_node_vector(node.key_pos);
        file.write_node_vector(node.child_pos);
        return file.get_pos();
    }

    template<typename K, typename V>
    typename BTree<K, V>::Node IOManager<K, V>::read_node(const int64_t pos) {
        Node node(t, false);
        read_node(&node, pos);
        return node;
    }

    template<typename K, typename V>
    void IOManager<K, V>::read_node(Node *node, const int64_t pos) {
        file.set_pos(pos);

        node->m_pos = pos;
        node->is_leaf = file.read_byte();
        node->used_keys = file.read_int16();
        file.read_node_vector(node->key_pos);
        file.read_node_vector(node->child_pos);
    }

    template<typename K, typename V>
    int64_t IOManager<K, V>::get_file_pos_end() {
        file.set_file_pos_to_end();
        return file.get_pos();
    }

    template<typename K, typename V>
    void IOManager<K, V>::validate(bool expression, const std::string &msg) const {
        if (!expression)
            throw std::logic_error(msg + " in " + file.path);
    }
}