#pragma once

#include "file_mapping.h"
#include "entry.h"

/**
 * Storage structures:
 *
 * - Header (13 bytes):
 *     - T                        |=> takes 2 bytes -> tree degree
 *     - KEY_SIZE                 |=> takes 1 byte
 *     - VALUE_TYPE               |=> takes 1 byte ->  VALUE_TYPE = 0 for primitives: (u)int32_t, (u)int64_t, float, double
 *                                                     VALUE_TYPE = 1 for container of values: (w)string, vectors<T>
 *                                                     VALUE_TYPE = 2 for blob
 *
 *     - ELEMENT_SIZE             |=> takes 1 byte  -> ELEMENT_SIZE = sizeof(VALUE_TYPE) for primitives
 *                                                     ELEMENT_SIZE = mask from the 8 bits (max 256 bytes) for non-primitives
 *     - ROOT POS                 |=> takes 8 bytes -> pos in file
 *
 * - Node (N bytes):
 *     - FLAG                     |=> takes 1 byte                 -> for "is_deleted" or "is_leaf"
 *     - USED_KEYS                |=> takes 2 bytes                -> for the number of "active" keys in the node
 *     - KEY_POS                  |=> takes (2 * t - 1) * KEY_SIZE -> for key positions in file
 *     - CHILD_POS                |=> takes (2 * t) * KEY_SIZE     -> for key positions in file
 *
 * - Entry (M bytes):
 *     - KEY                      |=> takes KEY_SIZE bytes (4 bytes is enough for 10^8 different keys)
 *     ----------–-----
 *        - VALUE                 |=> takes ELEMENT_SIZE bytes for primitive VALUE_TYPE
 *     or
 *        - NUMBER_OF_ELEMENTS    |=> takes 4 bytes
 *        - VALUES                |=> takes (ELEMENT_SIZE * NUMBER_OF_ELEMENTS) bytes
 *     ----------–-----
*/

template <typename K, typename V>
class BTree;

template <typename K, typename V>
struct IOManager {
    static constexpr int64_t INVALID_ROOT_POS = -1;

private:
    static constexpr int64_t HEADER_SIZE = 13;
    static constexpr int64_t ROOT_POS_IN_HEADER = 5;

    using EntryT = Entry<K,V>;
    using Node = typename BTree<K,V>::Node;

    MappedFile file;

public:
    explicit IOManager(const std::string& path);

    int64_t write_header(const int16_t t);
    int64_t read_header(const int16_t& user_t);

    int64_t write_node(const Node& node, const int64_t pos);
    void read_node(Node* node, const int64_t pos);

    void write_entry(const EntryT& entry, const int64_t pos);
    EntryT read_entry(const int64_t pos);

    K read_key(const int64_t pos);

    void write_flag(uint8_t flag, const int64_t pos);
    void write_new_pos_for_root_node(const int64_t posRoot);

    bool is_ready();
    int64_t get_file_pos_end();

private:
    void validate(bool expression, const std::string& msg);
};

#include "io_manager_impl.h"