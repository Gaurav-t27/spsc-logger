#pragma once
#include <cstddef>
#include <cstdint>

namespace logger {

enum class Level : uint8_t {
    TRACE = 0,
    DEBUG = 1,
    INFO  = 2,
    WARN  = 3,
    ERROR = 4,
    OFF   = 5,
};

constexpr const char* to_string(Level l) noexcept {
    switch (l) {
        case Level::TRACE: return "TRACE";
        case Level::DEBUG: return "DEBUG";
        case Level::INFO:  return "INFO ";
        case Level::WARN:  return "WARN ";
        case Level::ERROR: return "ERROR";
        default:           return "UNKNOWN";
    }
}


using DecodeFn = void (*)(const char* fmt,
                          const char* arg_buf,
                          char*       out,
                          size_t      out_len) noexcept;



struct Record {
    uint64_t    tsc;         // offset  0  raw RDTSC
    DecodeFn    decode_fn;   // offset  8  
    const char* fmt;         // offset 16  
    uint32_t    tid;         // offset 24  thread id
    Level       level;       // offset 28  
    uint8_t     arg_len;     // offset 29  bytes used in arg_buf
    uint8_t     pad_[2];     // offset 30  padding
    char        arg_buf[96]; // offset 32  encoded argument bytes
};                           // total    128 bytes

static_assert(sizeof(Record) == 128,
              "Record must be exactly 128 bytes (2 cache lines). "
              "Check field sizes and padding.");

static_assert(alignof(Record) == 8,
              "Record alignment must be 8 bytes.");

}
