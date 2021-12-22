#include <iostream>

using std::cout;
using std::endl;

#include "btree_impl.h"
#include "btree_node_impl.h"

#ifdef UNIT_TESTS
#if USE_BOOST_PREBUILT_STATIC_LIBRARY
#include <boost/test/unit_test.hpp>
#else
#include <boost/test/included/unit_test.hpp>
#endif

#include <boost/test/data/test_case.hpp>
#include <boost/range/iterator_range.hpp>


namespace {
    BOOST_AUTO_TEST_CASE(test_b_tree_init) {
        const int n = 1000;
        using BTreeIntInt = BTree<int, int>;
        bool found_all = false, any_not_found = false, remove_all = false;

        for (int order = 2; order < 101; ++order) {
            BTreeIntInt btree(order);
            for (int i = 0; i < n; ++i) {
                btree.set(i, 65 + i);
            }

            int total_found = 0;
            for (int i = 0; i < n; ++i) {
                total_found += btree.exist(i);
            }
            found_all = (total_found == n);

            int total_not_found = 0;
            const int key_shift = 1000;
            for (int i = 0; i < n; ++i) {
                total_not_found += !btree.exist(key_shift + i);
            }
            any_not_found = (total_not_found == n);

            int total_deleted = 0;
            for (int i = 0; i < n * 2; ++i) {
                total_deleted += btree.remove(i);
            }
            remove_all = (total_deleted == n);

            std::string msg = "BTree<int, int, " + std::to_string(order) + ">";
            BOOST_REQUIRE_MESSAGE(found_all && any_not_found && remove_all, msg);
        }
    }
}
#else

void at_exit_handler();

int main() {
    const int handler = std::atexit(at_exit_handler);

//    BTree<int, std::string> treeString(3);
//    BTree<int, char*> treeBlob(10);
//    cout << sizeof(std::unique_ptr<Node<int, int>>) << endl;
//    cout << sizeof(BTreeNode<int, int>*) << endl;
//    cout << sizeof(std::unique_ptr<int>) << endl;

    BTree<int, int> bTree5(5);

//    for (int i = 0; i < 50; ++i) {/*
//        bTree5.set(i, (char(65 + i))); // char for view in debugger
//    }*/
    for (int i = 0; i < 50; i += 2) {
        bTree5.set(i, char(65 + i)); // char for view in debugger
    }
//
    for (int i = 1; i < 50; i += 2) {
        bTree5.set(i, -165);
    }
//
    int found = 0;
    for (int i = 0; i < 50; ++i) {
        bool hasKey = bTree5.exist(i);
        if (!hasKey) {
            cout << "Can't find key: " << i << endl;
        }
        found += hasKey ? 1 : 0;
    }

    for (int i = 0; i < 50; ++i) {
        bool deleted = bTree5.remove(i);
        if (!deleted) {
            cout << "Can't delete key: " << i << endl;
        }
    }

    cout << "Total keys found: " << found << endl;

    cout << endl;
    cout << "Tree traversal" << endl;
    cout << "-----------------------------" << endl;
    bTree5.traverse();
    cout << "-----------------------------" << endl;
}
#endif // UNIT_TESTS