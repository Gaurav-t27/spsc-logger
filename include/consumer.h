#pragma once
#include "logger.h"
#include "tsc.h"
#include <atomic>
#include <cstdio>
#include <thread>

namespace logger {

class Consumer {
public:

    static constexpr size_t LINE_BUF_SIZE = 512;

    explicit Consumer(FILE* out = stderr)
        : out_(out),
          cal_(Tsc::Calibration::measure()) {
        running_.store(true, std::memory_order_relaxed);
        thread_ = std::thread(&Consumer::drainLoop, this);
    }

    Consumer(const Consumer&)            = delete;
    Consumer& operator=(const Consumer&) = delete;

    ~Consumer() {
        if (running_.exchange(false, std::memory_order_relaxed)) {
            if(thread_.joinable())
                thread_.join();
        }
    }

private:

    void drainLoop() noexcept {
        while (running_.load(std::memory_order_relaxed)) {
            size_t n = Logger::instance().drainAll([this](const Record& r) noexcept {
                formatAndWrite(r);
            });

            if (n == 0)
                std::this_thread::yield();
        }

        Logger::instance().drainAll([this](const Record& r) noexcept {
            formatAndWrite(r);
        });
    }

    void formatAndWrite(const Record& r) noexcept {
        char msg_buf[LINE_BUF_SIZE];
        r.decode_fn(r.fmt, r.arg_buf, msg_buf, sizeof(msg_buf));

        uint64_t ns = cal_.toNs(r.tsc);

        char line_buf[LINE_BUF_SIZE + 64];
        int  n = std::snprintf(line_buf, sizeof(line_buf),
                               "[%s] [%20llu] [%5u] %s\n",
                               r.level.toString(),
                               static_cast<unsigned long long>(ns),
                               r.tid,
                               msg_buf);

        if (n > 0)
            std::fwrite(line_buf, 1, static_cast<size_t>(n), out_);
    }

    FILE* out_;
    Tsc::Calibration cal_;
    std::atomic<bool> running_{false};
    std::thread thread_;
};

} // namespace logger
