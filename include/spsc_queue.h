#pragma once
#include <atomic>
#include <cstddef>
#include <optional>

namespace logger {

template<typename T, size_t Capacity>
class SPSCQueue {

    static_assert((Capacity & (Capacity - 1)) == 0,
                  "Capacity must be a power of 2");
    static_assert(Capacity >= 2,
                  "Capacity must be at least 2");

    static constexpr size_t Mask = Capacity - 1;

    alignas(64) T buffer_[Capacity];

    alignas(64) std::atomic<size_t> head_{0};

    alignas(64) std::atomic<size_t> tail_{0};

public:

    bool push(T val) noexcept {
        size_t h = head_.load(std::memory_order_relaxed);

        //Check if queue is full
        if (h - tail_.load(std::memory_order_acquire) == Capacity)
            return false;

        buffer_[h & Mask] = std::move(val);

        head_.store(h + 1, std::memory_order_release);
        return true;
    }

    std::optional<T> pop() noexcept {
        size_t t = tail_.load(std::memory_order_relaxed);

        //check if queue is empty
        if (t == head_.load(std::memory_order_acquire))
            return std::nullopt;

        T val = std::move(buffer_[t & Mask]);

        tail_.store(t + 1, std::memory_order_release);
        return val;
    }

    template<typename Fn>
    bool consume(Fn&& fn) noexcept {
        size_t t = tail_.load(std::memory_order_relaxed);

        if (t == head_.load(std::memory_order_acquire))
            return false;

        fn(buffer_[t & Mask]);

        tail_.store(t + 1, std::memory_order_release);
        return true;
    }

    template<typename Fn>
    size_t consumeAll(Fn&& fn) noexcept {
        size_t t     = tail_.load(std::memory_order_relaxed);
        size_t h     = head_.load(std::memory_order_acquire);
        size_t count = h - t;

        for (size_t i = 0; i < count; ++i)
            fn(buffer_[(t + i) & Mask]);

        if (count > 0)
            tail_.store(t + count, std::memory_order_release);

        return count;
    }

    bool empty() const noexcept {
        return head_.load(std::memory_order_acquire) ==
               tail_.load(std::memory_order_acquire);
    }

    size_t size() const noexcept {
        size_t h = head_.load(std::memory_order_acquire);
        size_t t = tail_.load(std::memory_order_acquire);
        return h - t;
    }

    static constexpr size_t capacity() noexcept { return Capacity; }
};

} // namespace logger
