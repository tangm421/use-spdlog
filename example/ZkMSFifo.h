#ifndef __ZKMSFIFO__
#define __ZKMSFIFO__

#include <cstdint>
#include <memory>
#include <chrono>
#include <deque>
#include <mutex>
#include <condition_variable>

template <typename T>
class ZKMSFifo
{
public:
    typedef T* pointer;
    typedef std::deque<pointer> MessageQueue;
    using size_t = typename MessageQueue::size_type;
    using msg_ptr_t = std::shared_ptr<T>;
    using scope_lock_t = std::lock_guard<std::mutex>;

    ZKMSFifo() {}
    virtual ~ZKMSFifo() {}

    ZKMSFifo(const ZKMSFifo&) = delete;
    ZKMSFifo& operator=(const ZKMSFifo&) = delete;

    bool empty() const {
        scope_lock_t locker(m_mutex_);
        return m_fifo_.empty();
    }

    virtual size_t size() const {
        scope_lock_t locker(m_mutex_);
        return m_fifo_.size();
    }

    /// remove all elements in the queue
    virtual void clear() {
        scope_lock_t locker(m_mutex_);
        m_fifo_.clear();
    };

    void getNext(pointer& msg) {
        std::unique_lock<std::mutex> ulk(m_mutex_);

        // Wait util there are messages available.
        m_condition_.wait(ulk, [this]() {return !m_fifo_.empty(); });

        // Return the first message on the fifo.
        //
        msg = m_fifo_.front();
        m_fifo_.pop_front();
    }

    msg_ptr_t getNext() {
        pointer msg;
        getNext(msg);
        return msg_ptr_t(msg);
    }

    template< class Rep, class Period>
    bool getNext(const std::chrono::duration<Rep, Period>& duration, pointer& msg) {
        std::unique_lock<std::mutex> ulk(m_mutex_);
        bool res = m_condition_.wait_for(ulk, duration, [this]() {return !m_fifo_.empty(); });
        if (!res)
        {// timeout
            return false;
        }
        else
        {
            msg = m_fifo_.front();
            m_fifo_.pop_front();
            return true;
        }
    }

    template< class Rep, class Period>
    msg_ptr_t getNext(const std::chrono::duration<Rep, Period>& duration) {
        pointer msg;
        if (getNext(duration, msg))
        {
            return msg_ptr_t(msg);
        }
        else
        {
            return msg_ptr_t();
        }
    }

    size_t add(const pointer& item) {
        scope_lock_t locker(m_mutex_);
        m_fifo_.push_back(item);
        m_condition_.notify_one();
        return m_fifo_.size();
    }

protected:
    MessageQueue m_fifo_;
    mutable std::mutex m_mutex_;
    std::condition_variable m_condition_;
};
#endif // #ifndef __ZKMSFIFO__
