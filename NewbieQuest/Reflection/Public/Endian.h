#pragma once
#include <cstdint>
#include <cstring>
#include <type_traits>
#if __has_include(<bit>)
  #include <bit> // std::endian (C++20)
#endif

namespace qreflect {

// byte swap(supporting 2/4/8)
template <class T>
inline T byteswap(T v) {
    static_assert(std::is_trivially_copyable_v<T>, "byteswap requires trivially copyable type");
    if constexpr (sizeof(T) == 1) {
        return v;
    } else if constexpr (sizeof(T) == 2) {
        uint16_t x; std::memcpy(&x, &v, 2);
        x = static_cast<uint16_t>((x << 8) | (x >> 8));
        std::memcpy(&v, &x, 2);
        return v;
    } else if constexpr (sizeof(T) == 4) {
        uint32_t x; std::memcpy(&x, &v, 4);
        x = ((x & 0x000000FFu) << 24) |
            ((x & 0x0000FF00u) << 8)  |
            ((x & 0x00FF0000u) >> 8)  |
            ((x & 0xFF000000u) >> 24);
        std::memcpy(&v, &x, 4);
        return v;
    } else if constexpr (sizeof(T) == 8) {
        uint64_t x; std::memcpy(&x, &v, 8);
        x =  (x >> 56)
           | ((x >> 40) & 0x000000000000FF00ull)
           | ((x >> 24) & 0x0000000000FF0000ull)
           | ((x >>  8) & 0x00000000FF000000ull)
           | ((x <<  8) & 0x000000FF00000000ull)
           | ((x << 24) & 0x0000FF0000000000ull)
           | ((x << 40) & 0x00FF000000000000ull)
           |  (x << 56);
        std::memcpy(&v, &x, 8);
        return v;
    } else {
        // extend it if needed
        return v;
    }
}

// little endian fixation save/restore
template <class T>
inline T to_le(T v) {
#if defined(__cpp_lib_endian)
    if constexpr (std::endian::native == std::endian::little) {
        return v;
    } else {
        return byteswap(v);
    }
#else
    #ifdef _WIN32
        return v; // MSVC/x86 uses little endian
    #else
        // conservative swap (adjustable as desired environment)
        return byteswap(v);
    #endif
#endif
}

template <class T>
inline T from_le(T v) {
    // same logic as to_le (symmetry)
#if defined(__cpp_lib_endian)
    if constexpr (std::endian::native == std::endian::little) {
        return v;
    } else {
        return byteswap(v);
    }
#else
    #ifdef _WIN32
        return v;
    #else
        return byteswap(v);
    #endif
#endif
}

}
