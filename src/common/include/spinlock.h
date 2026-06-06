#pragma once

#include <atomic>
#include <chrono>

namespace nebula {
class SpinLock {
public:
    SpinLock() = default;
    ~SpinLock() = default;

    // 禁止拷贝和移动
    SpinLock(const SpinLock &) = delete;
    SpinLock &operator=(const SpinLock &) = delete;
    SpinLock(SpinLock &&) = delete;
    SpinLock &operator=(SpinLock &&) = delete;

    void lock();
    void unlock();

private:
    std::atomic<uint32_t> next_tickets_{0};    // 叫号
    std::atomic<uint32_t> serving_tickets_{0};  // 服务号
};

class RwSpinLock {
public:
    RwSpinLock() = default;
    ~RwSpinLock() = default;

    // 禁止拷贝和移动
    RwSpinLock(const RwSpinLock &) = delete;
    RwSpinLock &operator=(const RwSpinLock &) = delete;
    RwSpinLock(RwSpinLock &&) = delete;
    RwSpinLock &operator=(RwSpinLock &&) = delete;

    void lock();
    bool try_lock_for(const std::chrono::milliseconds &timeout);
    void unlock();

    void lock_shared();
    bool try_lock_shared_for(const std::chrono::milliseconds &timeout);
    void unlock_shared();

private:
    std::atomic<uint32_t> lock_word_{0};  // 最高位为写锁标志，次高位为等待写锁标志，剩余位是读锁计数
    static const uint32_t WRITE_FLAG;
    static const uint32_t WAIT_WRITE_FLAG;

    bool try_once_wlock(uint32_t &spin_count);
    bool try_once_rlock(uint32_t &spin_count);
};

} // namespace nebula