//#include "test.pb.h"
#include "test.protoflat.h"

#include <protoflat.h>

#include <iomanip>
#include <iostream>

struct Message
{
    int32_t id;
    std::string text;
    std::vector<int32_t> data;
    std::vector<std::string> messages;

    bool operator!=(const Message &another) const
    {
        return std::tie(id, text, data, messages) != std::tie(another.id, another.text, another.data, messages);
    }
};

//namespace protoflat
//{

//template<>
//struct type_traits<Message>
//{
//    inline static constexpr field_header id_header{1, wire_type::varint};
//    inline static constexpr field_header text_header{2, wire_type::length_delimited};
//    inline static constexpr field_header data_header{3, wire_type::length_delimited};
//    inline static constexpr field_header messages_header{4, wire_type::length_delimited};

//    static size_t size(const Message &value)
//    {
//        size_t size = 0;

//        size += type_traits<varint>::size(field_header::encode(id_header));
//        size += type_traits<varint>::size(value.id);

//        size += type_traits<varint>::size(field_header::encode(text_header));
//        size += type_traits<std::string>::size(value.text);

//        size += type_traits<varint>::size(field_header::encode(data_header));
//        size += type_traits<std::vector<varint>>::size(value.data);

//        for (auto &message : value.messages)
//        {
//            size += type_traits<varint>::size(field_header::encode(messages_header));
//            size += type_traits<std::string>::size(message);
//        }

//        return size;
//    }

//    static void serialize(const Message &value, std::string &data)
//    {
//        if (value.id)
//        {
//            type_traits<varint>::serialize(field_header::encode(id_header), data);
//            type_traits<varint>::serialize(value.id, data);
//        }

//        if (!value.text.empty())
//        {
//            type_traits<varint>::serialize(field_header::encode(text_header), data);
//            type_traits<std::string>::serialize(value.text, data);
//        }

//        if (!value.data.empty())
//        {
//            type_traits<varint>::serialize(field_header::encode(data_header), data);
//            type_traits<std::vector<varint>>::serialize(value.data, data);
//        }

//        if (!value.messages.empty())
//        {
//            for (auto &message : value.messages)
//            {
//                type_traits<varint>::serialize(field_header::encode(messages_header), data);
//                type_traits<std::string>::serialize(message, data);
//            }
//        }
//    }

//    static bool deserialize(std::string_view &data, Message &value)
//    {
//        while (true)
//        {
//            uint64_t header_value = 0;
//            if (!type_traits<varint>::deserialize(data, header_value))
//            {
//                break;
//            }

//            auto header = field_header::decode(header_value);
//            switch (header.field_number)
//            {
//            case 1:
//                if (header.field_type != id_header.field_type || !type_traits<varint>::deserialize(data, value.id))
//                {
//                    return false;
//                }
//                break;
//            case 2:
//                if (header.field_type != text_header.field_type || !type_traits<std::string>::deserialize(data, value.text))
//                {
//                    return false;
//                }
//                break;
//            case 3:
//                if (header.field_type != data_header.field_type || !type_traits<std::vector<varint>>::deserialize(data, value.data))
//                {
//                    return false;
//                }
//            case 4:
//            {
//                std::string message;
//                if (header.field_type != messages_header.field_type || !type_traits<std::string>::deserialize(data, message))
//                {
//                    return false;
//                }
//                value.messages.push_back(std::move(message));
//            }
//            default:
//                break;
//            }
//        }

//        return true;
//    }
//};

//}

void print(const std::string &data)
{
    for (auto c : data)
    {
        std::cout << std::hex << std::setw(2) << std::setfill('0') << (0xff & static_cast<uint32_t>(c)) << " ";
    }
    std::cout << std::endl;
}

int main()
{
    //    Message message{0, "Hello!", {}, {"Hello!", "World!"}};
    //    auto data = protoflat::serialize(message);
    //    print(data);

    //    Message message_deserialized;
    //    std::string_view data_view(data);
    //    if (!protoflat::deserialize(data_view, message_deserialized) || message != message_deserialized)
    //    {
    //        std::cout << "failed" << std::endl;
    //    }

    test::Message message2;
    //    message2.set_id(0);
    //    message2.set_text("Hello!");
    //    message2.add_data(3);
    //    message2.add_data(270);
    //    message2.add_data(86942);
    //    message2.add_messages("Hello!");
    //    message2.add_messages("World!");

    //    print(message2.SerializeAsString());

    return 0;
}
