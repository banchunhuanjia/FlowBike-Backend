#ifndef PACKET_HEADER_H
#define PACKET_HEADER_H

// 【关键修正】包含定义了 uint32_t 和 size_t 的头文件
#include <cstdint> // 为了 uint32_t
#include <cstddef> // 为了 size_t

#pragma pack(push, 1)
struct PacketHeader {
    uint32_t length;
    uint32_t type_id;
};
#pragma pack(pop)

const size_t HEADER_SIZE = sizeof(PacketHeader);

#endif // PACKET_HEADER_H