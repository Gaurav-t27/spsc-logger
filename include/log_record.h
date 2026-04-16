#pragma once
#include <cstddef>
#include <cstdint>

namespace logger {

class Level {
public:
    enum class Value : uint8_t {
        TRACE = 0,
        DEBUG = 1,
        INFO  = 2,
        WARN  = 3,
        ERROR = 4,
        OFF   = 5,
    };

    constexpr Level(Value v) noexcept : value_(v) {}

    constexpr const char* to_string() const noexcept {
        switch (value_) {
            case Value::TRACE: return "TRACE";
            case Value::DEBUG: return "DEBUG";
            case Value::INFO:  return "INFO ";
            case Value::WARN:  return "WARN ";
            case Value::ERROR: return "ERROR";
            default:           return "?????";
        }
    }

    constexpr bool operator>=(Level rhs) const noexcept {
        return static_cast<uint8_t>(value_) >=
               static_cast<uint8_t>(rhs.value_);
    }

    constexpr uint8_t underlying() const noexcept {
        return static_cast<uint8_t>(value_);
    }

private:
    Value value_;
};

static_assert(sizeof(Level) == 1, "Level must be 1 byte.");


struct Record {

    using DecodeFn = void (*)(const char* fmt,
                              const char* arg_buf,
                              char*       out,
                              size_t      out_len) noexcept;

    uint64_t    tsc;         // offset  0  raw RDTSC
    DecodeFn    decode_fn;   // offset  8  
    const char* fmt;         // offset 16  
    uint32_t    tid;         // offset 24  thread id
    Level       level;       // offset 28  log level
    uint8_t     arg_len;     // offset 29  bytes used in arg_buf
    uint8_t     pad_[2];     // offset 30  explicit padding
    char        arg_buf[96]; // offset 32  encoded argument bytes
};                           // total    128 bytes

static_assert(sizeof(Record) == 128,
              "Record must be exactly 128 bytes (2 cache lines). "
              "Check field sizes and padding.");

static_assert(alignof(Record) == 8,
              "Record alignment must be 8 bytes.");

} // namespace logger
