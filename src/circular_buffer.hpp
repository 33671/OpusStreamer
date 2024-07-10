#ifndef CIRCULAR_BUFFER_HPP
#define CIRCULAR_BUFFER_HPP

#include <atomic>
#include <condition_variable>
#include <list>
#include <memory>
#include <mutex>
#include <stdexcept>
#include <vector>

// BufferClosedException implementation
class BufferClosedException : public std::runtime_error {
public:
    explicit BufferClosedException(const std::string& message)
        : std::runtime_error(message)
    {
    }
};

template <typename T>
class CircularBuffer {
public:
    explicit CircularBuffer(size_t size);
    CircularBuffer(size_t size, const T& place_holder);

    CircularBuffer(const CircularBuffer&) = delete;
    CircularBuffer& operator=(const CircularBuffer&) = delete;

    void push(const T& item);
    void push_no_wait(const T& item);
    T pop();
    void abort();

    size_t queue_size() const;
    size_t capacity();

    bool is_full() const;
    bool is_empty() const;

private:
    std::atomic_bool exit_flag { false };
    std::vector<T> buffer;
    size_t head;
    size_t tail;
    size_t size_;
    bool full;
    mutable std::mutex mutex;
    std::condition_variable cond_not_full;
    std::condition_variable cond_not_empty;
};

template <typename T>
class CircularBufferBroadcast {
    static_assert(std::is_default_constructible<T>::value, "T must be default constructible");

public:
    explicit CircularBufferBroadcast(size_t ring_size);
    void broadcast(const T& item);
    std::shared_ptr<CircularBuffer<T>> subscribe();
    void unsubscire(std::shared_ptr<CircularBuffer<T>> buffer);

private:
    size_t ring_size_;
    std::list<std::shared_ptr<CircularBuffer<T>>> subscribers_;
    mutable std::mutex mutex;
};

// #include "circular_buffer.tpp"

#endif // CIRCULAR_BUFFER_HPP
