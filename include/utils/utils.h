#pragma once

#include <string>
#include <vector>

namespace utils {
#if _WIN64 || __amd64__
    static_assert(sizeof(int64_t) == sizeof(size_t));
#else
    static_assert(sizeof(int32_t) == sizeof(size_t));
#endif
    template <typename T>
    void shift_right_by_one(std::vector<T>& v, const int32_t from, const int32_t to) {
        for (auto i = from; i > to; --i) {
            v[i] = v[i - 1];
        }
    }

    template <typename T>
    void shift_left_by_one(std::vector<T>& v, const int32_t from, const int32_t to) {
        for (auto i = from; i < to; ++i) {
            v[i - 1] = v[i];
        }
    }

    template <typename T>
    struct identity_type {
        using type = std::remove_pointer_t<T>;
    };

    template <typename T>
    struct underlying_type {
        using type = typename T::value_type;
    };

    template <bool Condition>
    using enable_if_t = typename std::enable_if<Condition, bool>::type;

    template <bool Condition, typename TrueType, typename FalseType>
    using conditional_t = typename std::conditional_t<Condition, TrueType, FalseType>;

    template <typename T>
    struct is_string {
        static constexpr bool value = false;
    };

    template <typename CharT, typename Traits, typename Alloc>
    struct is_string<std::basic_string<CharT, Traits, Alloc>> {
        static constexpr bool value = true;
    };

    template <typename T>
    inline constexpr bool is_string_v = is_string<T>::value;

    template <typename V>
    constexpr uint8_t get_element_size() {
        if constexpr (std::is_arithmetic_v<V>) {
            return sizeof(V);
        } else {
            if constexpr (is_string_v<V>)
                return sizeof(typename V::value_type);
            else {
                static_assert(std::is_pointer_v<V>);
                return sizeof(std::remove_pointer_t<V>);
            }
        }
    }

    enum ValueTypes: uint8_t {
        INTEGER = 0,
        UNSIGNED_INTEGER = 1,
        FLOATING_POINT = 2,
        BASIC_STRING = 3,
        BLOB = 3
    };

    template <typename V>
    constexpr uint8_t get_value_type_code() {
        if constexpr (std::is_integral_v<V>) {
            return std::is_unsigned_v<V> ? ValueTypes::UNSIGNED_INTEGER : ValueTypes::INTEGER;
        } else {
            if constexpr (std::is_floating_point_v<V>) {
                return ValueTypes::FLOATING_POINT;
            } else {
                if constexpr (is_string_v<V>) {
                    return ValueTypes::BASIC_STRING;
                } else {
                    static_assert(std::is_pointer_v<V>);
                    return ValueTypes::BLOB;
                }
            }
        }
    }

    template <typename PtrT>
    constexpr uint8_t* cast_to_uint8_t_data(PtrT t) {
        return reinterpret_cast<uint8_t*>(t);
    }

    template <typename PtrT>
    constexpr const uint8_t* cast_to_const_uint8_t_data(PtrT t) {
        return reinterpret_cast<const uint8_t*>(t);
    }

    void validate(bool expression, const std::string_view& err_msg, const std::string& file_path) {
        if (!expression)
            // todo: fix "Крэш с гарантией, если `err_msg` не заканчивается `\0`: [MP review]"
            throw std::logic_error(err_msg.data() + file_path);
    }
}