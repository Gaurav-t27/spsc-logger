#include <atomic>
#include <optional>

template<typename T, size_t capacity>
class SPSCQueue {
    alignas(64) T buffer_[capacity];
    alignas(64) std::atomic<size_t> head_{0};
    alignas(64) std::atomic<size_t> tail_{0};
public:
    
    bool push(T val) {
        size_t curr_head = head_.load(std::memory_order_relaxed);
        size_t next_head = (curr_head + 1) % capacity;

        //check if the queue is full
        if(next_head == tail_.load(std::memory_order_acquire)) {
            return false;
        }
        buffer_[curr_head] = std::move(val);
        head_.store(next_head, std::memory_order_release);
        return true;
    }

    std::optional<T> pop() {
        size_t curr_tail = tail_.load(std::memory_order_relaxed);

        //check if the queue is empty
        if(curr_tail == head_.load(std::memory_order_acquire)) {
            return std::nullopt;
        }
        T val = std::move(buffer_[curr_tail]);
        size_t next_tail = (curr_tail + 1) % capacity;
        tail_.store(next_tail, std::memory_order_release);
        return val;
    }

    size_t size() const {
        size_t h = head_.load(std::memory_order_acquire);
        size_t t = tail_.load(std::memory_order_acquire);
        return (h - t + capacity) % capacity;
    }

    bool empty() const {
        return head_.load(std::memory_order_acquire) == tail_.load(std::memory_order_acquire);
    }
};