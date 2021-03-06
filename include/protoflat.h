#pragma once

#include <string>
#include <string_view>
#include <type_traits>
#include <vector>

namespace protoflat
{

namespace zigzag
{

inline constexpr uint64_t encode(int64_t value)
{
    return (value << 1) ^ (value >> 63);
}

inline constexpr int64_t decode(uint64_t value)
{
    return (value >> 1) ^ -(value & 1);
}

} // namespace zigzag

enum class wire_type : uint8_t
{
    varint,
    fixed64,
    length_delimited,
    start_group,
    end_group,
    fixed32
};

inline std::string_view wire_type_string(wire_type type)
{
    switch (type)
    {
    case wire_type::varint:
        return "varint";
    case wire_type::fixed64:
        return "fixed64";
    case wire_type::length_delimited:
        return "length_delimited";
    case wire_type::start_group:
        return "start_group";
    case wire_type::end_group:
        return "end_group";
    case wire_type::fixed32:
        return "fixed32";
    }
}

struct field_header
{
    uint64_t field_number;
    wire_type field_type;

    static constexpr uint64_t encode(field_header field)
    {
        return (field.field_number << 3) | static_cast<uint8_t>(field.field_type);
    }

    static constexpr field_header decode(uint64_t value)
    {
        return field_header{value >> 3, wire_type(value & 0x7)};
    }
};

struct varint
{
};

struct fixed
{
};

struct length_delimited
{
};

struct packed_varint
{
};

struct packed_fixed
{
};

inline std::string_view protoflat_specialization_type(wire_type type, bool is_packed)
{
    switch (type)
    {
    case wire_type::varint:
        return is_packed ? "packed_varint" : "varint";
    case wire_type::length_delimited:
        return "length_delimited";
    case wire_type::fixed32:
    case wire_type::fixed64:
        return is_packed ? "packed_fixed" : "fixed";
    default:
        return "unknown";
    }
}

template<class T>
struct type_traits
{
    static size_t size(T value);
    static void serialize(T value, std::string &data);
    static bool deserialize(std::string_view &data, T &value);
};

template<>
struct type_traits<varint>
{
    template<class T, typename = std::enable_if_t<std::is_integral_v<T> || std::is_enum_v<T>>>
    static size_t size(T source_value)
    {
        uint64_t &value = reinterpret_cast<uint64_t &>(source_value);
        size_t size = 0;
        do
        {
            value >>= 7;
            ++size;
        } while (value > 0);

        return size;
    }

    template<class T, typename = std::enable_if_t<std::is_integral_v<T> || std::is_enum_v<T>>>
    static void serialize(T source_value, std::string &data)
    {
        uint64_t &value = reinterpret_cast<uint64_t &>(source_value);
        do
        {
            char byte = value & 0x7f;
            value >>= 7;
            if (value > 0)
            {
                byte |= 0x80;
            }
            data += byte;
        } while (value > 0);
    }

    template<class T, typename = std::enable_if_t<std::is_integral_v<T>>>
    static bool deserialize(std::string_view &data, T &value)
    {
        bool result = false;
        size_t offset = 0;
        for (auto &c : data)
        {
            uint64_t byte = c;
            reinterpret_cast<uint64_t &>(value) |= (byte & 0x7f) << 7 * offset;

            ++offset;
            if (byte <= 0x7f)
            {
                result = true;
                break;
            }
        }

        data.remove_prefix(offset);

        return result;
    }
};

template<>
struct type_traits<fixed>
{
    template<class T, typename = std::enable_if_t<std::is_arithmetic_v<T> && sizeof(T) >= 4>>
    static size_t size(T)
    {
        return sizeof(T);
    }

    template<class T, typename = std::enable_if_t<std::is_arithmetic_v<T> && sizeof(T) >= 4>>
    static void serialize(T source_value, std::string &data)
    {
        uint64_t &value = reinterpret_cast<uint64_t &>(source_value);
        for (int i = 0; i < sizeof(T); ++i)
        {
            data += static_cast<char>(value & 0xff);
            value >>= 8;
        }
    }

    template<class T, typename = std::enable_if_t<std::is_arithmetic_v<T> && sizeof(T) >= 4>>
    static bool deserialize(std::string_view &data, T &value)
    {
        if (data.size() < sizeof(T))
        {
            return false;
        }

        for (int i = 0; i < sizeof(T); ++i)
        {
            value |= data[i];
            value <<= 8;
        }
        data.remove_prefix(sizeof(T));

        return true;
    }
};

template<>
struct type_traits<length_delimited>
{
    static size_t size(const std::string &value)
    {
        return value.size();
    }

    static void serialize(const std::string &value, std::string &data)
    {
        type_traits<varint>::serialize(type_traits::size(value), data);
        data += value;
    }

    static bool deserialize(std::string_view &data, std::string &value)
    {
        uint64_t size = 0;
        if (type_traits<varint>::deserialize(data, size))
        {
            if (data.size() >= size)
            {
                value.append(data.substr(0, size));
                data.remove_prefix(size);

                return true;
            }
        }

        return false;
    }
};

template<>
struct type_traits<packed_varint>
{
    template<class T, typename = std::enable_if_t<std::is_integral_v<T> || std::is_enum_v<T>>>
    static size_t size(const std::vector<T> &values)
    {
        size_t size = 0;
        for (const auto &value : values)
        {
            size += type_traits<varint>::size(value);
        }

        return size;
    }

    template<class T, typename = std::enable_if_t<std::is_integral_v<T> || std::is_enum_v<T>>>
    static void serialize(const std::vector<T> &values, std::string &data)
    {
        type_traits<varint>::serialize(type_traits::size(values), data);
        for (const auto &value : values)
        {
            type_traits<varint>::serialize(value, data);
        }
    }

    template<class T, typename = std::enable_if_t<std::is_integral_v<T> || std::is_enum_v<T>>>
    static bool deserialize(std::string_view &data, std::vector<T> &values)
    {
        uint64_t size;
        if (type_traits<varint>::deserialize(data, size))
        {
            if (data.size() >= size)
            {
                size_t until = data.size() - size;
                while (data.size() > until)
                {
                    T value;
                    if (type_traits<varint>::deserialize(data, value))
                    {
                        values.push_back(value);
                    }
                    else
                    {
                        return false;
                    }
                }

                return true;
            }
        }

        return false;
    }
};

template<>
struct type_traits<packed_fixed>
{
    template<class T, typename = std::enable_if_t<std::is_arithmetic_v<T> && sizeof(T) >= 4>>
    static size_t size(const std::vector<T> &values)
    {
        return values.size() * sizeof(T);
    }

    template<class T, typename = std::enable_if_t<std::is_arithmetic_v<T> && sizeof(T) >= 4>>
    static void serialize(const std::vector<T> &values, std::string &data)
    {
        type_traits<varint>::serialize(type_traits::size(values), data);
        for (auto &value : values)
        {
            type_traits<fixed>::serialize(value, data);
        }
    }

    template<class T, typename = std::enable_if_t<std::is_arithmetic_v<T> && sizeof(T) >= 4>>
    static bool deserialize(std::string_view &data, std::vector<T> &values)
    {
        uint64_t size;
        if (type_traits<varint>::deserialize(data, size))
        {
            if (data.size() >= size)
            {
                size_t until = data.size() - size;
                while (data.size() > until)
                {
                    T value;
                    if (type_traits<fixed>::deserialize(data, value))
                    {
                        values.push_back(value);
                    }
                    else
                    {
                        return false;
                    }
                }

                return true;
            }
        }

        return false;
    }
};

template<class T, typename = std::enable_if_t<std::is_class_v<T>>>
inline void serialize_to_string(const T &value, std::string &data)
{
    auto size = type_traits<T>::size(value);
    if (data.capacity() < size)
    {
        data.reserve(size);
    }
    type_traits<T>::serialize(value, data);
}

template<class T, typename = std::enable_if_t<std::is_class_v<T>>>
inline std::string serialize(const T &value)
{
    std::string data;
    serialize_to_string(value, data);
    return data;
}

template<class T, typename = std::enable_if_t<std::is_class_v<T>>>
inline bool deserialize(std::string_view &data, T &value)
{
    value = {};
    return type_traits<T>::deserialize(data, value);
}

} // namespace protoflat
