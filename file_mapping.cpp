#include <filesystem>

#include "file_mapping.h"

namespace fs = std::filesystem;

MappedFile::MappedFile(const std::string &fn, int64_t bytes_num) : path(fn), m_pos(0) {
    bool file_exists = fs::exists(fn);
    if (!file_exists) {
        std::filebuf fbuf;
        fbuf.open(path, std::ios_base::out | std::ios_base::trunc);
        fbuf.pubseekoff(bytes_num, std::ios_base::beg);
        fbuf.sputc(0);
        fbuf.close();
        m_size = m_capacity = bytes_num;
    } else {
        m_size = m_capacity = static_cast<int64_t>(fs::file_size(fn));
    }
    remap();
}

MappedFile::~MappedFile() {
#ifdef _MSC_VER_
    fs::resize_file(path,m_capacity);
#endif
}

template <typename T>
T MappedFile::read_next() {
    static_assert(std::is_arithmetic_v<T>);
    char *value_begin = mapped_region_begin + m_pos;
    m_pos += sizeof(T);
    return *(reinterpret_cast<T*>(value_begin));
}

template <typename T>
void MappedFile::write(T val, int64_t f_pos) {
    static_assert(std::is_arithmetic_v<T>);
    m_pos = write_to_dst(val, f_pos * sizeof(T));
    m_capacity = std::max(m_pos, m_capacity);
}

template <typename T>
void MappedFile::write_next(T val) {
    static_assert(std::is_arithmetic_v<T>);
    m_pos = write_to_dst(val, m_pos);
    m_capacity = std::max(m_pos, m_capacity);
}

template <typename T>
std::int64_t MappedFile::write_to_dst(T val, int64_t dst) {
    int64_t total_size = sizeof(T);
    if (m_pos + total_size > m_size) {
        resize(std::max(2 * m_size, total_size));
    }
    char* data = reinterpret_cast<char *>(&val);
    std::copy(data, data + total_size, mapped_region_begin + dst);
    return dst + total_size;
}

void MappedFile::resize(int64_t new_size) {
    m_size = new_size;

    std::filebuf fbuf;
    fbuf.open(path, std::ios_base::in | std::ios_base::out);
    fbuf.pubseekoff(m_size, std::ios_base::beg);
    fbuf.sputc(0);
    fbuf.close();

    remap();
}

void MappedFile::remap() {
    auto new_mapping = bip::file_mapping(path.data(), bip::read_write);
    auto new_region = bip::mapped_region(new_mapping, bip::read_write);
    file_mapping.swap(new_mapping);
    mapped_region.swap(new_region);
    mapped_region_begin = reinterpret_cast<char *>(mapped_region.get_address());
}

void MappedFile::write_int_array(int* vec, const std::int32_t used) {
    int64_t total_size = static_cast<int64_t>(sizeof(int32_t)) * used;
    if (m_pos + total_size > m_size) {
        resize(std::max(2 * m_size, total_size));
    }
    const char* data = reinterpret_cast<const char *>(vec);
    std::copy(data, data + total_size, mapped_region_begin + m_pos);
    m_pos += total_size;
    m_capacity = std::max(m_pos, m_capacity);
}

void MappedFile::read_int_array(int* vec, int32_t used){
    uint32_t total_size = sizeof(int32_t) * used;
//    assert(used <= static_cast<int32_t>(vec.size()));

    char* data = reinterpret_cast<char *>(vec);
    char* start = mapped_region_begin + m_pos;
    char* end = start + total_size;
    std::copy(start, end, data);
    m_pos += total_size;
}

void MappedFile::setPosFile(int64_t pos) {
    m_pos = pos > 0 ? pos : 0;
}

int32_t MappedFile::read_int() {
    return read_next<int32_t>();
}

int64_t MappedFile::getPosFile() {
    return m_pos;
}

void MappedFile::write_int(const int32_t val) {
    write_next(val);
}

void MappedFile::write_byte(uint8_t val) {
    write_next(val);
}

uint8_t MappedFile::read_byte() {
    return read_next<uint8_t>();
}

void MappedFile::setPosEndFile() {
    m_pos = m_capacity;
}

bool MappedFile::isEmpty() {
    return m_size == 0;
}

//#ifdef UNIT_TESTS
//#if USE_BOOST_PREBUILT_STATIC_LIBRARY
//#include <boost/test/unit_test.hpp>
//#else
//#include <boost/test/included/unit_test.hpp>
//#endif
//
//#include <boost/test/data/test_case.hpp>
//#include <boost/range/iterator_range.hpp>


void test_create_and_write() {
    std::string fmap = std::string("../file_mapping_text") + std::string(".txt");
    if (fs::exists(fmap)) {
        fs::remove(fmap);
    }

    MappedFile file(fmap, 32);
    for (int i = 0; i < 1000000; ++i) {
        file.write_next(i);
    }
}

void test_read() {
    std::string fmap = std::string("../file_mapping_text") + std::string(".txt");
    MappedFile file(fmap, 32);
    for (int i = 0; i < 1000000; ++i) {
        auto anInt = file.read_int();
        assert(i == anInt);
    }
}

void test_modify_and_save() {
    std::string fmap = std::string("../file_mapping_text") + std::string(".txt");
    {
        MappedFile file(fmap, 32);
        for (int i = 0; i < 1000000; i += 1000) {
            file.write(-1, i);
        }
    }
    {
        MappedFile file(fmap, 32);
        for (int i = 0; i < 1000000; ++i) {
            auto anInt = file.read_int();
            if (i % 1000 == 0) {
                assert(anInt == -1);
            } else {
                assert(i == anInt);
            }
        }
    }
}

void test_array() {
    std::string fmap = std::string("../file_mapping_array") + std::string(".txt");
    if (fs::exists(fmap)) {
        fs::remove(fmap);
    }

    int n = 1000000;
//    std::vector<int> out(n, 1);
//    std::vector<int> in(n, 0);
    {
        MappedFile file(fmap, 32);
//        file.write_int_array(out, n);
    }

    {
        MappedFile file(fmap, 32);
//        file.read_int_array(in, n);
    }
//    assert(in == out);
}

//namespace {
//    BOOST_AUTO_TEST_CASE(file_mapping_test) {
int main1() {
//        std::string fn = std::string("../save") + std::strin g(".txt");
    test_create_and_write();
//    test_modify_and_save();
//    test_array();

    std::string msg = "File mapping test";
//        BOOST_REQUIRE_MESSAGE(success, msg);
return 1;
}
//    }
//}
//#endif