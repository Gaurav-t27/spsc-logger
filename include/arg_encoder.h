#pragma once
#include "log_record.h"
#include <algorithm>
#include <cstring>
#include <cstdio>
#include <tuple>
#include <type_traits>

namespace logger {


class Encoder {
public:

    static constexpr uint8_t ARG_BUF_SIZE = 96; 

    static void encodeArg(char* buf, uint8_t& pos, const char* s) noexcept {
        if (!s) s = "";

        uint8_t avail = ARG_BUF_SIZE - pos;
        if (avail < 2) return; // need at least length byte + null

        uint8_t max_len = avail - 2;
        uint8_t len = static_cast<uint8_t>(std::min(std::strlen(s), static_cast<size_t>(max_len)));

        buf[pos++] = len;                // store length
        std::memcpy(buf + pos, s, len);      // store content
        buf[pos + len] = '\0';               // null terminate
        pos += len + 1;
    }

    template<typename T>
    static void encodeArg(char* buf, uint8_t& pos, T val) noexcept {
        using DT = std::decay_t<T>;

        static_assert(std::is_trivially_copyable_v<DT>,
                      "Log arguments must be trivially copyable or const char*. "
                      "std::string and similar heap-owning types are not supported.");

        if (static_cast<size_t>(ARG_BUF_SIZE - pos) < sizeof(DT)) return;  // not enough room — skip

        std::memcpy(buf + pos, &val, sizeof(DT));
        pos += static_cast<uint8_t>(sizeof(DT));
    }
};

template<typename T>
std::decay_t<T> decodeOne(const char* buf, size_t& pos) noexcept {
    using DT = std::decay_t<T>;

    if constexpr (std::is_same_v<DT, const char*>) {
        uint8_t     len = static_cast<uint8_t>(buf[pos++]);
        const char* ptr = buf + pos;
        pos += len + 1;
        return ptr;

    } else {
        DT val;
        std::memcpy(&val, buf + pos, sizeof(DT));
        pos += sizeof(DT);
        return val;
    }
}

template<typename... Args>
std::tuple<Args...> decodeAll([[maybe_unused]] const char* buf, [[maybe_unused]] size_t& pos) noexcept {
    return std::tuple<Args...>{ decodeOne<Args>(buf, pos)... };
}


class Decoder {
public:
    template<typename... Args>
    static Record::DecodeFn makeDecoder() noexcept {
        return [](const char* fmt, const char* buf, char* out, size_t out_len) noexcept
        {
            size_t pos = 0;

            auto decoded = decodeAll<Args...>(buf, pos);

            std::apply([&](auto&&... args) noexcept {
                std::snprintf(out, out_len, fmt, args...);
            }, decoded);
        };
    }
};

} // namespace logger
