#pragma once

#include <filesystem>
#include <iostream>

#include "utils/utils.h"

namespace fs = std::filesystem;

namespace btree {
namespace file {
    void seek_file_to_offset(const std::string& path, const std::ios_base::openmode file_open_mode, const int64_t offset) {
        std::filebuf buf;
        auto* p_file_buf = buf.open(path, file_open_mode);
        if (!p_file_buf)
            throw std::runtime_error("Wrong path is provided for mapped file, path = " + path);
        buf.pubseekoff(offset, std::ios_base::beg);
        if (offset > 0) // if file is not empty it needs EOF
            buf.sputc(0);
        buf.close();
    }
}
    using namespace utils;

    template <typename K, typename V>
    MappedFile<K,V>::MappedFile(const std::string& path, const int64_t bytes_num) :
            m_pos(0), m_mapped_region(new MappedRegion()), path(path)
    {
        bool file_exists = fs::exists(path);
        if (!file_exists) {
            file::seek_file_to_offset(path, std::ios_base::out | std::ios_base::trunc, bytes_num);
            m_size = m_capacity = bytes_num;
        } else {
            m_size = m_capacity = static_cast<int64_t>(fs::file_size(path));
        }
        if (m_size > 0)
            m_mapped_region->remap(path);
    }

    template <typename K, typename V>
    MappedFile<K,V>::~MappedFile() {
/**  _MSC_VER
 * 1. Faced with the same error as in: https://youtrack.jetbrains.com/issue/PROF-752
    - [WIN32 error] = 1224, The requested operation cannot be performed on a file with a user-mapped section open.

 * 2. See impl of std::filesystem::resize_file
    - https://github.com/microsoft/STL/blob/main/stl/src/filesystem.cpp#L671
       - Use WinAPI BOOL SetEndOfFile([in] HANDLE hFile) for Windows platform
 * 3. Doc https://docs.microsoft.com/en-us/windows/win32/api/fileapi/nf-fileapi-setendoffile
    - CreateFileMapping is called to create a file mapping object for hFile,
      UnmapViewOfFile must be called first to unmap all views and call CloseHandle to close
      the file mapping object before you can call SetEndOfFile.
 * 4. See impl of BOOST_MAPPED_REGION dtor:
    - https://github.com/steinwurf/boost/blob/master/boost/interprocess/mapped_region.hpp#L555
*/
        std::error_code error_code;
        fs::resize_file(path, m_capacity, error_code);
        if (error_code)
            std::cerr << "Can't resize file: " + path << std::endl;
    }

    template <typename K, typename V>
    MappedFile<K,V>::MappedRegion::MappedRegion() : mapped_region_begin(nullptr) {}

    template <typename K, typename V>
    uint8_t* MappedFile<K,V>::MappedRegion::address_by_offset(const int64_t offset) const {
        return mapped_region_begin + offset;
    }

    template <typename K, typename V>
    void MappedFile<K,V>::MappedRegion::remap(const std::string& file_path) {
        auto file_mapping = bip::file_mapping(file_path.data(), bip::read_write);
        auto tmp_mapped_region = bip::mapped_region(file_mapping, bip::read_write);
        mapped_region.swap(tmp_mapped_region);
        mapped_region_begin = cast_to_uint8_t_data(mapped_region.get_address());
    }

    template <typename K, typename V>
    void MappedFile<K,V>::write_next_data(ValueType val, const int32_t total_size_in_bytes) {
        if constexpr(std::is_pointer_v<ValueType>)
            m_pos = write_blob(val, total_size_in_bytes);
        else
            m_pos = write_arithmetic(val);
        m_capacity = std::max(m_pos, m_capacity);
    }

    template <typename K, typename V>
    template <typename ValueType>
    std::pair<ValueType, int32_t> MappedFile<K,V>::read_next_data() {
        if constexpr(std::is_pointer_v<ValueType>) {
            auto len = read_next_primitive<int32_t>();
            auto* value_begin = m_mapped_region->address_by_offset(m_pos);
            m_pos += len;
            return std::make_pair(cast_to_const_uint8_t_data(value_begin), len);
        } else {
            static_assert(std::is_arithmetic_v<ValueType>);
            return std::make_pair(read_next_primitive<ValueType>(), static_cast<int32_t>(sizeof(ValueType)));
        }
    }

    template <typename K, typename V>
    template <typename T>
    void MappedFile<K,V>::write_next_primitive(const T val) {
        static_assert(std::is_arithmetic_v<T>);
        m_pos = write_arithmetic(val);
        m_capacity = std::max(m_pos, m_capacity);
    }

    template <typename K, typename V>
    template <typename T>
    T MappedFile<K,V>::read_next_primitive() {
        static_assert(std::is_arithmetic_v<T>);
        auto* value_begin = m_mapped_region->address_by_offset(m_pos);
        m_pos += sizeof(T);
        return *(reinterpret_cast<T*>(value_begin));
    }

    template <typename K, typename V>
    template <typename T>
    void MappedFile<K,V>::write_node_vector(const std::vector<T>& vec) {
        int64_t total_size_in_bytes = sizeof(T) * vec.size();
        if (m_pos + total_size_in_bytes > m_size)
            resize(m_pos + total_size_in_bytes);

        auto* data = cast_to_const_uint8_t_data(vec.data());
        std::copy(data, data + total_size_in_bytes, m_mapped_region->address_by_offset(m_pos));
        m_pos += total_size_in_bytes;
        m_capacity = std::max(m_pos, m_capacity);
    }

    template <typename K, typename V>
    template <typename T>
    void MappedFile<K,V>::read_node_vector(std::vector<T>& vec) {
        int64_t total_size = sizeof(T) * vec.size();

        auto* data = cast_to_uint8_t_data(vec.data());
        auto* start = m_mapped_region->address_by_offset(m_pos);
        auto* end = start + total_size;
        std::copy(start, end, data);
        m_pos += total_size;
    }

    template <typename K, typename V>
    template <typename T>
    int64_t MappedFile<K,V>::write_arithmetic(T val) {
        static_assert(std::is_arithmetic_v<T>);
        int64_t total_size_in_bytes = sizeof(T);
        if (m_pos + total_size_in_bytes > m_size)
            resize(m_pos + total_size_in_bytes);

        auto* data = cast_to_const_uint8_t_data(&val);
        std::copy(data, data + total_size_in_bytes, m_mapped_region->address_by_offset(m_pos));
        return m_pos + total_size_in_bytes;
    }

    template <typename K, typename V>
    template <typename T>
    int64_t MappedFile<K,V>::write_blob(T source_data, const int32_t total_size_in_bytes) {
        // write size
        int32_t len = total_size_in_bytes;
        m_pos = write_arithmetic(len);

        // write values
        int64_t total_bytes_size = total_size_in_bytes;
        if (m_pos + total_bytes_size > m_size)
            resize(m_pos + total_bytes_size);

        auto* data = cast_to_const_uint8_t_data(source_data);
        std::copy(data, data + total_bytes_size, m_mapped_region->address_by_offset(m_pos));
        return m_pos + total_bytes_size;
    }

    template <typename K, typename V>
    void MappedFile<K,V>::resize(int64_t new_size, bool shrink_to_fit) {
        // Can't use std::filesystem::resize_file(), see file_mapping_impl.h: ~MappedFile() {...}
        m_size = shrink_to_fit ? new_size : std::max(scale_current_size(), new_size);
        file::seek_file_to_offset(path, std::ios_base::in | std::ios_base::out, m_size);
        m_mapped_region->remap(path);
    }

    template <typename K, typename V>
    void MappedFile<K,V>::set_pos(int64_t pos) {
        m_pos = pos > 0 ? pos : 0;
    }

    template <typename K, typename V>
    int16_t MappedFile<K,V>::read_int16() {
        return read_next_primitive<int16_t>();
    }

    template <typename K, typename V>
    int32_t MappedFile<K,V>::read_int32() {
        return read_next_primitive<int32_t>();
    }

    template <typename K, typename V>
    int64_t MappedFile<K,V>::read_int64() {
        return read_next_primitive<int64_t>();
    }

    template <typename K, typename V>
    int64_t MappedFile<K,V>::get_pos() const {
        return m_pos;
    }

    template <typename K, typename V>
    uint8_t MappedFile<K,V>::read_byte() {
        return read_next_primitive<uint8_t>();
    }

    template <typename K, typename V>
    void MappedFile<K,V>::set_file_pos_to_end() {
        m_pos = m_capacity;
    }

    template <typename K, typename V>
    void MappedFile<K,V>::shrink_to_fit() {
        m_capacity = m_size = m_pos;
        resize(m_size, true);
        m_mapped_region->remap(path);
    }

    template <typename K, typename V>
    bool MappedFile<K,V>::is_empty() const {
        return m_size == 0;
    }
}