#pragma once
#include "log_record.h"
#include "spsc_queue.h"
#include "tsc.h"
#include "arg_encoder.h"
#include <atomic>
#include <unistd.h>

namespace logger {

class Logger {
public:

    static constexpr size_t QUEUE_CAPACITY = 16384;  // must be power of 2
    using Queue = SPSCQueue<Record, QUEUE_CAPACITY>;

    static Logger& instance() {
        static Logger logger;
        return logger;
    }

    void setLevel(Level l) noexcept {
        min_level_.store(l.underlying(), std::memory_order_relaxed);
    }

    bool isEnabled(Level l) const noexcept {
        return l >= Level(Level::Value(min_level_.load(std::memory_order_relaxed)));
    }

    Queue& queue() noexcept { return queue_; }

    template<typename... Args>
    void write(Level l, const char* fmt, Args&&... args) noexcept {
        if (!isEnabled(l)) return;

        Record r;
        r.tsc = Tsc::now();
        r.tid = threadId();
        r.level = l;
        r.fmt = fmt;
        r.decode_fn = Decoder::makeDecoder<std::decay_t<Args>...>();
        r.arg_len = 0;

        (Encoder::encodeArg(r.arg_buf, r.arg_len, std::forward<Args>(args)), ...);

        queue_.push(r);
    }

    template<typename... Args>
    void trace(const char* fmt, Args&&... args) noexcept {
        write(Level(Level::Value::TRACE), fmt, std::forward<Args>(args)...);
    }

    template<typename... Args>
    void debug(const char* fmt, Args&&... args) noexcept {
        write(Level(Level::Value::DEBUG), fmt, std::forward<Args>(args)...);
    }

    template<typename... Args>
    void info(const char* fmt, Args&&... args) noexcept {
        write(Level(Level::Value::INFO), fmt, std::forward<Args>(args)...);
    }

    template<typename... Args>
    void warn(const char* fmt, Args&&... args) noexcept {
        write(Level(Level::Value::WARN), fmt, std::forward<Args>(args)...);
    }

    template<typename... Args>
    void error(const char* fmt, Args&&... args) noexcept {
        write(Level(Level::Value::ERROR), fmt, std::forward<Args>(args)...);
    }

private:

    static uint32_t threadId() noexcept {
        static thread_local uint32_t tid = static_cast<uint32_t>(::gettid());
        return tid;
    }

    Queue queue_;
    std::atomic<uint8_t> min_level_{ static_cast<uint8_t>(Level::Value::INFO) };
};

} // namespace logger
