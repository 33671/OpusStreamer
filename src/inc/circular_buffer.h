#include <condition_variable>
#include <list>
#include <memory>
#include <mutex>
#include <vector>
#ifndef CIRCULAR_BUFFER
#define CIRCULAR_BUFFER
template <typename T>
class CircularBuffer {
public:
    CircularBuffer(size_t size)
        : buffer(size)
        , head(0)
        , tail(0)
        , full(false)
        , size_(size)
    {
    }
     CircularBuffer(size_t size,const T& place_holder)
        : head(0)
        , tail(0)
        , full(false)
        , size_(size)
    {
        buffer = std::vector<T>(size,place_holder);
    }

    CircularBuffer(const CircularBuffer&) = delete;

    CircularBuffer& operator=(const CircularBuffer&) = delete;

    void push(const T& item)
    {
        std::unique_lock<std::mutex> lock(mutex);
        cond_not_full.wait(lock, [this] { return !full; });

        buffer[head] = item;
        head = (head + 1) % buffer.size();
        full = head == tail;

        lock.unlock();
        cond_not_empty.notify_one();
    }

    void push_no_wait(const T& item)
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

    T pop()
    {
        std::unique_lock<std::mutex> lock(mutex);
        cond_not_empty.wait(lock, [this] { return !is_empty(); });

        T item = buffer[tail];
        tail = (tail + 1) % buffer.size();
        full = false;

        lock.unlock();
        cond_not_full.notify_one();

        return item;
    }

    size_t queue_size() const
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

    size_t capacity()
    {
        return size_;
    }

    bool is_full() const { return full; }
    bool is_empty() const { return (!full && (head == tail)); }

private:
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
public:
    explicit CircularBufferBroadcast(size_t ring_size)
        : ring_size_(ring_size)
    {
    }
    void broadcast(const T& item)
    {
        // std::unique_lock<std::mutex> lock(mutex);
        for (auto buff : subscribers_) {
            buff.get()->push_no_wait(item);
        }
    }
    std::shared_ptr<CircularBuffer<T>> subscribe()
    {
        auto buffer = std::make_shared<CircularBuffer<T>>(ring_size_,255);
        // std::unique_lock<std::mutex> lock(mutex);
        subscribers_.push_back(buffer);
        return buffer;
    }
    void unsubscire(std::shared_ptr<CircularBuffer<T>> buffer)
    {
        // std::unique_lock<std::mutex> lock(mutex);
        subscribers_.remove(buffer);
    }

private:
    size_t ring_size_;
    std::list<std::shared_ptr<CircularBuffer<T>>> subscribers_;
    // mutable std::mutex mutex;
};
#endif
