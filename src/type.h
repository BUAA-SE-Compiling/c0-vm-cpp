#ifndef TYPE_H_INCLUDED
#define TYPE_H_INCLUDED

#include <cstdint>
#include <string>
#include <type_traits>

namespace vm {

using u1 = std::uint8_t;
using u2 = std::uint16_t;
using u4 = std::uint32_t;
using u8 = std::uint64_t;

#define U1_MAX UINT8_MAX
#define U2_MAX UINT16_MAX
#define U4_MAX UINT32_MAX
#define U8_MAX UINT64_MAX

using i1 = std::int8_t;
using i2 = std::int16_t;
using i4 = std::int32_t;
using i8 = std::int64_t;

using f4 = float;
using f8 = double;

using slot_t   = i4;
using int_t    = slot_t;
using double_t = f8;
using addr_t   = slot_t;
using char_t   = unsigned char;
using str_t    = std::string;

template <typename T>
// sizeof(T)/sizeof(slot_t)
constexpr int_t slots_count = sizeof(T)/sizeof(slot_t);

/*
template <class T>
struct is_vm_supported : std::false_type {};
*/
}

#endif