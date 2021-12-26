#include "file_mapping.h"
#include "entry.h"
#include "btree.h"

template <typename K, typename V>
struct IOManager {
    static constexpr int INVALID_ROOT_POS = -1;

    using EntryT = Entry<K,V>;
    using Node = typename BTree<K,V>::Node;

    MappedFile file;

    explicit IOManager(const std::string& path) : file(path, 0) {}

    bool is_ready() {
        return !file.isEmpty();
    }

    void write_entry(const EntryT& entry, const int pos)    {
        file.set_pos(pos);

        file.write_next(entry.key);
        file.write_next(entry.value);
    }

    K read_key(const int pos) {
        file.set_pos(pos);

        K key = file.read_next<K>();
        assert(key > -1);
        return key;
    }

    EntryT read_entry(const int pos) {
        file.set_pos(pos);

        K key = file.read_next<K>();
        V value = file.read_next<V>();
        assert(key > -1);
        return { key, value };
    }


    void write_flag(char flag, const int pos) {
        file.set_pos(pos);

        file.write_next(flag);
    }

    int read_header(int& t) {
        file.set_pos(0);

        t = file.read_int();
//        auto key_size = file.read_next<uint8_t>();
//        auto curr_value_type = file.read_next<uint8_t>();
//        auto cur_value_type_size = file.read_next<uint8_t>();
//
//        assert(key_size == sizeof(K));
//        assert(curr_value_type == value_type<V>());
//        assert(cur_value_type_size == value_type_size<V>());

        int posRoot = file.read_int();
        return posRoot;
    }

    int write_header(const int t) {
        file.set_pos(0);
//        uint8_t val = value_type_size<V>();
//        uint8_t i = value_type<V>();
//        assert((sizeof(t) + 1 + i + val) == 5);
//        const int root_pos = 6;

        file.write_next(t);                             // 2 bytes
//        file.write_next<uint8_t>(sizeof(K));            // 1 byte
//        file.write_next<uint8_t>(i);                    // 1 byte
//        file.write_next<uint8_t>(val);                      // 1 byte
        file.write_next(8);                      // 4 byte -> write root pos
        return file.get_pos();
    }

    void writeUpdatePosRoot(const int posRoot) {
        file.set_pos(4);

        file.write_next(posRoot);
    }

    int write_node(const Node& node, const int pos) {
        file.set_pos(pos);

        file.write_next(node.flag);
        file.write_next(node.used_keys);
        file.write_next(node.key_pos);
        file.write_next(node.child_pos);
        return file.get_pos();
    }

    void read_node(Node* node, const int pos) {
        file.set_pos(pos);

        node->m_pos = pos;
        node->flag = file.read_byte();
        node->used_keys = file.read_int();
        file.read_vector(node->key_pos);
        file.read_vector(node->child_pos);
    }

    int get_file_pos_end() {
        file.set_file_pos_to_end();
        return file.get_pos();
    }
};