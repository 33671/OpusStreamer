#ifndef CIRCULAR_BUFFER_TPP
#define CIRCULAR_BUFFER_TPP

#include "circular_buffer.hpp"

// CircularBuffer implementation

template <typename T>
CircularBuffer<T>::CircularBuffer(size_t size)
    : buffer(size)
    , head(0)
    , tail(0)
    , full(false)
    , size_(size)
{
}

template <typename T>
CircularBuffer<T>::CircularBuffer(size_t size, const T& place_holder)
    : head(0)
    , tail(0)
    , full(false)
    , size_(size)
{
    buffer = std::vector<T>(size, place_holder);
}

template <typename T>
void CircularBuffer<T>::push(const T& item)
{
    std::unique_lock<std::mutex> lock(mutex);
    cond_not_full.wait(lock, [this] { return !full || exit_flag.load(); });
    if (exit_flag.load()) {
        throw BufferClosedException("Aborted");
    }

    buffer[head] = item;
    head = (head + 1) % buffer.size();
    full = head == tail;

    lock.unlock();
    cond_not_empty.notify_one();
}

template <typename T>
void CircularBuffer<T>::push_no_wait(const T& item)
{
    std::unique_lock<std::mutex> lock(mutex);
    if (full) {
        tail = (tail + 1) % buffer.size(); // 如果满了，覆盖最早的元素
    }

    buffer[head] = item;
    head = (head + 1) % buffer.size();
    full = head == tail;

    lock.unlock();
    cond_not_empty.notify_one();
}

template <typename T>
T CircularBuffer<T>::pop()
{
    std::unique_lock<std::mutex> lock(mutex);
    cond_not_empty.wait(lock, [this] { return !is_empty() || exit_flag.load(); });
    if (exit_flag.load()) {
        throw BufferClosedException("Aborted");
    }
    T item = buffer[tail];
    tail = (tail + 1) % buffer.size();
    full = false;

    lock.unlock();
    cond_not_full.notify_one();

    return item;
}

template <typename T>
void CircularBuffer<T>::abort()
{
    printf("Aborting\n");
    exit_flag.store(true);
    cond_not_empty.notify_all();
    cond_not_full.notify_all();
}

template <typename T>
size_t CircularBuffer<T>::queue_size() const
{
    std::unique_lock<std::mutex> lock(mutex);
    if (full) {
        return buffer.size();
    } else if (head >= tail) {
        return head - tail;
    } else {
        return buffer.size() - tail + head;
    }
}

template <typename T>
size_t CircularBuffer<T>::capacity()
{
    return size_;
}

template <typename T>
bool CircularBuffer<T>::is_full() const
{
    return full;
}

template <typename T>
bool CircularBuffer<T>::is_empty() const
{
    return (!full && (head == tail));
}

// CircularBufferBroadcast implementation

template <typename T>
CircularBufferBroadcast<T>::CircularBufferBroadcast(size_t ring_size)
    : ring_size_(ring_size)
{
}

template <typename T>
void CircularBufferBroadcast<T>::broadcast(const T& item)
{
    std::unique_lock<std::mutex> lock(mutex);
    for (auto buff : subscribers_) {
        buff->push_no_wait(item);
    }
}

template <typename T>
std::shared_ptr<CircularBuffer<T>> CircularBufferBroadcast<T>::subscribe()
{
    auto buffer = std::make_shared<CircularBuffer<T>>(ring_size_);
    std::unique_lock<std::mutex> lock(mutex);
    subscribers_.push_back(buffer);
    return buffer;
}

template <typename T>
void CircularBufferBroadcast<T>::unsubscire(std::shared_ptr<CircularBuffer<T>> buffer)
{
    buffer->abort();
    std::unique_lock<std::mutex> lock(mutex);
    subscribers_.remove(buffer);
}

#endif // CIRCULAR_BUFFER_TPP
