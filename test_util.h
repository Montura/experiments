#include <string>
#include <map>

#include "btree.h"

using BTreeIntInt = BTree<int32_t, int32_t>;


class TestStat {
    const int64_t N;
public:
    int64_t total_found = 0;
    int64_t total_not_found = 0;
    int64_t total_removed = 0;
    int64_t total_after_remove = 0;
    int64_t total_after_reopen = 0;

    TestStat(int64_t N) : N(N) {}

    bool contains_all() const {
        return total_found == N;
    }

    bool any_not_found() const {
        return total_not_found == 0;
    }

    bool check_deleted(int32_t expected) const {
        return total_removed == expected;
    }

    bool found_all_the_remaining() const {
        return total_after_remove == (N -  total_removed);
    }
};


template <typename K, typename V>
std::map <K, V> test_keys_create_exist(const std::string& path, int order, int total_elements) {
    BTree<K,V> btree(path, order);

    TestStat stat(total_elements);
    std::map <K, V> verify_map;
    for (int i = 0; i < total_elements; ++i) {
        K key = i;
        V value = i + 65;
        btree.set(key, value);
        verify_map[key] = value;
    }

    for (int i = 0; i < total_elements; ++i)
        stat.total_found += btree.exist(i);
    assert(stat.contains_all());

    K max_key = verify_map.rbegin()->first + 1;
    for (int i = 0; i < total_elements; ++i)
        stat.total_not_found += btree.exist(max_key + i);

    assert(stat.any_not_found());
    return verify_map;
}

template <typename K, typename V>
int64_t test_values_get(const std::string& path, int order, int total_elements, const std::map <K, V>& verify_map) {
    BTree<K,V> btree(path, order);

    TestStat stat(total_elements);
    for (int i = 0; i < total_elements; ++i) {
        auto expected_value = verify_map.find(i);
        auto actual_value = btree.get(i);
        assert(expected_value->second == actual_value);
        ++stat.total_found;
    }
    assert(stat.contains_all());
    return stat.total_found;
}

template <typename K, typename V>
std::pair<int64_t, int64_t> test_values_remove(const std::string& path, int order,
                                               int total_elements, std::map <K, V>& verify_map)
{
    BTree<K,V> btree(path, order);

    int r1 = std::rand() % 13 + 1;
    int r2 = std::rand() % 31 + 1;
    int r3 = std::rand() % 73 + 1;

    TestStat stat(total_elements);
    for (int i = 0; i < total_elements; i += r1) {
        stat.total_removed += btree.remove(i);
        verify_map.erase(i);
    }

    for (int i = 0; i < total_elements; i += r2) {
        stat.total_removed += btree.remove(i);
        verify_map.erase(i);
    }

    for (int i = 0; i < 50; ++i) {
        int v1 = 0;
        verify_map.erase(v1);
        stat.total_removed += btree.remove(v1);

        int v2 = 3 * r2;
        verify_map.erase(v2);
        stat.total_removed += btree.remove(v2);

        int v3 = 7 * r3;
        verify_map.erase(v3);
        stat.total_removed += btree.remove(v3);
    }

    for (int i = 0; i < total_elements; ++i) {
        stat.total_after_remove += btree.exist(i);
    }
    assert(stat.total_after_remove == static_cast<int64_t>(verify_map.size()));
    assert(stat.found_all_the_remaining());

    return std::make_pair(stat.total_removed, stat.total_after_remove);
}

template <typename K, typename V>
void test_values_after_remove(const std::string& path, int order, int total_elements, const std::map <K, V>& verify_map) {
    BTree<K,V> btree(path, order);

    TestStat stat(total_elements);
    for (int i = 0; i < total_elements; ++i) {
        auto expected_value = verify_map.find(i);
        auto actual_value = btree.get(i);
        if (expected_value == verify_map.end()) {
            assert(actual_value == 0);
        } else {
            assert(expected_value->second == actual_value);
            ++stat.total_after_reopen;
        }
    }
    assert(stat.total_after_reopen == static_cast<int64_t>(verify_map.size()));
}
