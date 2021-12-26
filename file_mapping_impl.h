#include <filesystem>

#include "utils.h"
#include "file_mapping.h"

namespace fs = std::filesystem;

template <typename T>
T MappedFile::read_next() {
    if constexpr(std::is_arithmetic_v<T>) {
        static_assert(std::is_arithmetic_v<T>);
        char *value_begin = mapped_region_begin + m_pos;
        m_pos += sizeof(T);
        return *(reinterpret_cast<T *>(value_begin));
    } else {
        return read_string<T>();
    }
}

template <typename T>
T MappedFile::read_string() {
    int32_t elem_count = read_next<typename T::size_type>();
    T str(elem_count, '\0');

    uint32_t total_size = sizeof(typename T::value_type) * elem_count;

    char* data = reinterpret_cast<char *>(str.data());
    char* start = mapped_region_begin + m_pos;
    char* end = start + total_size;
    std::copy(start, end, data);
    m_pos += total_size;
    return str;
}

template <typename T>
void MappedFile::write_next(T val) {
    if constexpr(std::is_arithmetic_v<T>) {
        m_pos = write_arithmetic_to_dst(val, m_pos);
        m_capacity = std::max(m_pos, m_capacity);
    } else {
        if constexpr(is_string_v<T>) {
            m_pos = write_string_to_dst(val, m_pos);
            m_capacity = std::max(m_pos, m_capacity);
        } else {
            write_vector(val);
        }
    }
}

template <typename T>
void MappedFile::write(T val, int64_t f_pos) {
    static_assert(std::is_arithmetic_v<T>);
    m_pos = write_arithmetic_to_dst(val, f_pos * sizeof(T));
    m_capacity = std::max(m_pos, m_capacity);
}

template <typename T>
void MappedFile::read_vector(std::vector<T>& vec) {
    uint32_t total_size = sizeof(T) * vec.size();
//    assert(used <= static_cast<int32_t>(vec.size()));

    char* data = reinterpret_cast<char *>(vec.data());
    char* start = mapped_region_begin + m_pos;
    char* end = start + total_size;
    std::copy(start, end, data);
    m_pos += total_size;
};

template <typename T>
void MappedFile::write_vector(const std::vector<T>& vec) {
    int64_t total_size = static_cast<int64_t>(sizeof(T)) * vec.size();
    if (m_pos + total_size > m_size) {
        resize(std::max(2 * m_size, total_size));
    }
    const char* data = reinterpret_cast<const char *>(vec.data());
    std::copy(data, data + total_size, mapped_region_begin + m_pos);
    m_pos += total_size;
    m_capacity = std::max(m_pos, m_capacity);
}

template <typename T>
std::int64_t MappedFile::write_arithmetic_to_dst(T val, int64_t dst) {
    int64_t total_size = sizeof(T);
    if (m_pos + total_size > m_size) {
        resize(std::max(2 * m_size, total_size));
    }
    char* data = reinterpret_cast<char *>(&val);
    std::copy(data, data + total_size, mapped_region_begin + dst);
    return dst + total_size;
}

template <typename T>
std::int64_t MappedFile::write_string_to_dst(T val, int64_t dst) {
    // write size
    size_t elem_count = val.size();
    dst = write_arithmetic_to_dst(elem_count, dst);

    // write values
    int64_t total_bytes_size = sizeof(typename T::value_type) * elem_count;
    if (m_pos + total_bytes_size > m_size) {
        resize(std::max(2 * m_size, total_bytes_size));
    }

    char* data = reinterpret_cast<char *>(val.data());
    std::copy(data, data + total_bytes_size, mapped_region_begin + dst);
    return dst + total_bytes_size;
}

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
#ifndef _MSC_VER_
    fs::resize_file(path,m_capacity);
#endif
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

void MappedFile::set_pos(int64_t pos) {
    m_pos = pos > 0 ? pos : 0;
}

int32_t MappedFile::read_int() {
    return read_next<int32_t>();
}

int64_t MappedFile::get_pos() {
    return m_pos;
}

uint8_t MappedFile::read_byte() {
    return read_next<uint8_t>();
}

void MappedFile::set_file_pos_to_end() {
    m_pos = m_capacity;
}

bool MappedFile::isEmpty() {
    return m_size == 0;
}