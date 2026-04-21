#pragma once
#include "log_record.h"
#include "spsc_queue.h"
#include "tsc.h"
#include "arg_encoder.h"
#include <atomic>
#include <memory>
#include <mutex>
#include <unistd.h>
#include <vector>

namespace logger {

class Logger {
public:

    static constexpr size_t QUEUE_CAPACITY = 16384;
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

        threadQueue().push(r);
    }

    template<typename Fn>
    size_t drainAll(Fn&& fn) noexcept {
        std::lock_guard lock(mutex_);
        size_t total = 0;
        for (auto& q : queues_)
            total += q->consumeAll(fn);
        return total;
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

    struct QueueHandle {
        Queue* q;
        ~QueueHandle() { Logger::instance().deregister(q); }
    };

    Queue& threadQueue() noexcept {
        static thread_local QueueHandle handle{ registerThisThread() };
        return *handle.q;
    }

    Queue* registerThisThread() {
        auto   q   = std::make_unique<Queue>();
        Queue* ptr = q.get();
        std::lock_guard lock(mutex_);
        queues_.push_back(std::move(q));
        return ptr;
    }

    void deregister(Queue* ptr) {
        std::lock_guard lock(mutex_);
        std::erase_if(queues_, [ptr](const auto& q) { return q.get() == ptr; });
    }

    static uint32_t threadId() noexcept {
        static thread_local uint32_t tid = static_cast<uint32_t>(::gettid());
        return tid;
    }

    std::mutex mutex_;
    std::vector<std::unique_ptr<Queue>> queues_;
    std::atomic<uint8_t> min_level_{ static_cast<uint8_t>(Level::Value::INFO) };
};

} // namespace logger
