#pragma once

#include <filesystem>

#include "utils/boost_include.h"
#include "utils/utils.h"

namespace fs = std::filesystem;

namespace btree {
    using namespace utils;

    class MappedRegion {
        const std::string path;
        bip::offset_t mapped_offset;
        bip::mapped_region mapped_region;
        uint8_t* mapped_region_begin;
        bip::offset_t curr_pos;

        uint8_t* read_only_address_for_offset(int64_t offset) {
            auto end_address = curr_pos + offset;
            if (end_address > static_cast<int64_t>(mapped_region.get_size())) {
                remap(bip::read_only, calc_new_size(end_address));
                end_address = offset;
            }
            uint8_t* address_begin = address_by_offset(curr_pos);
            curr_pos = end_address;
            return address_begin;
        }

        int64_t calc_new_size(int64_t end_address) const {
            int64_t fileSize = static_cast<int64_t>(fs::file_size(path));
            if (end_address <= fileSize) {
//                int64_t delta = end_address - m_pos;
                int64_t new_size = 128;
                if (curr_pos + new_size > fileSize) {
                    return fileSize - curr_pos;
                } else {
                    return std::max(new_size, end_address);
                }
            } else {
                throw std::runtime_error("Attempted to read from memory outside the mapped region");
            }
        }

    public:
        explicit MappedRegion(int64_t file_pos,  const std::string& path);
        uint8_t* address_by_offset(const int64_t offset) const;
        void remap(bip::mode_t mode = bip::read_write, size_t size = 0);

        template <typename T>
        T read_next_primitive();

        template <typename ValueType>
        std::pair<ValueType, int32_t> read_next_data();

        template <typename T>
        int64_t write_next_primitive(const T val);

        template <typename T>
        int64_t write_blob(T source_data, const int32_t total_size_in_bytes);

        int64_t current_pos() const {
            return std::max(curr_pos, mapped_offset);
        }

        int64_t size() const {
            return static_cast<int64_t>(mapped_region.get_size());
        }
    };

    MappedRegion::MappedRegion(int64_t file_pos, const std::string& path)
            : path(path), mapped_offset(file_pos), mapped_region_begin(nullptr), curr_pos(0) {}

    uint8_t* MappedRegion::address_by_offset(int64_t offset) const {
        return mapped_region_begin + offset;
    }

    void MappedRegion::remap(bip::mode_t mode, size_t size) {
        if (mode == bip::read_only) {
            mapped_offset += curr_pos;
        }
        curr_pos = 0;
        auto file_mapping = bip::file_mapping(path.data(), mode);
        auto tmp_mapped_region = bip::mapped_region(file_mapping, mode, mapped_offset, size);
        mapped_region.swap(tmp_mapped_region);
        mapped_region_begin = cast_to_uint8_t_data(mapped_region.get_address());
    }

    template <typename T>
    T MappedRegion::read_next_primitive() {
        static_assert(std::is_arithmetic_v<T>);
        auto being_address = read_only_address_for_offset(sizeof(T));
        return *(reinterpret_cast<T*>(being_address));
    }

    template <typename ValueType>
    std::pair<ValueType, int32_t> MappedRegion::read_next_data() {
        if constexpr(std::is_pointer_v<ValueType>) {
            auto len = read_next_primitive<int32_t>();
            auto being_address = read_only_address_for_offset(len);
            return std::make_pair(cast_to_const_uint8_t_data(being_address), len);
        } else {
            static_assert(std::is_arithmetic_v<ValueType>);
            return std::make_pair(read_next_primitive<ValueType>(), static_cast<int32_t>(sizeof(ValueType)));
        }
    }

    template <typename T>
    int64_t MappedRegion::write_next_primitive(const T val) {
        static_assert(std::is_arithmetic_v<T>);
        int64_t total_size_in_bytes = sizeof(T);

        auto* data = cast_to_const_uint8_t_data(&val);
        std::copy(data, data + total_size_in_bytes, address_by_offset(curr_pos));
        mapped_offset += total_size_in_bytes;
        curr_pos = mapped_offset;
        return curr_pos;
    }

    template <typename T>
    int64_t MappedRegion::write_blob(T source_data, const int32_t total_size_in_bytes) {
        auto* data = cast_to_const_uint8_t_data(source_data);
        std::copy(data, data + total_size_in_bytes, address_by_offset(curr_pos));
        mapped_offset += total_size_in_bytes;
        curr_pos = mapped_offset;
        return curr_pos;
    }

}