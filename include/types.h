#ifndef PROJECT_TYPES_H
#define PROJECT_TYPES_H

// 引入 C++ 标准的固定宽度整数头文件
#include <cstdint>

// --- 统一的类型别名定义 ---

// 无符号整数
using u8  = std::uint8_t;
using u16 = std::uint16_t;
using u32 = std::uint32_t;
using u64 = std::uint64_t;

// 有符号整数
using i8  = std::int8_t;
using i16 = std::int16_t;
using i32 = std::int32_t;
using i64 = std::int64_t;

// (可选) 为其他常用类型定义别名
//#include <string>
//using String = std::string;

// 更多...

#endif // PROJECT_TYPES_H