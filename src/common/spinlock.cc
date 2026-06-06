#include <thread>
#include <xmmintrin.h>
#include "db_define.h"
#include "spinlock.h"

namespace nebula {
// 三阶段退避策略
static inline void delay(uint32_t &spin_count)
{
    const uint32_t first_phase = 50;
    const uint32_t sec_phase = 500;
    if (spin_count < first_phase) {
        for (int i = 0; i < 10; ++i) {
            _mm_pause();
        }
        spin_count++;
    } else if (spin_count < sec_phase) {
        std::this_thread::yield();
        spin_count++;
    } else {
        std::this_thread::sleep_for(std::chrono::microseconds(1));
    }
}
// SpinLock implementation
void SpinLock::lock() {
    uint32_t spin_count = 0;
    uint32_t ticket = next_tickets_.fetch_add(1, std::memory_order_relaxed);
    while (serving_tickets_.load(std::memory_order_acquire) != ticket) {
        // 三阶段退避策略
        delay(spin_count);
    }
}

void SpinLock::unlock() {
    serving_tickets_.fetch_add(1, std::memory_order_release);
}

// RwSpinLock implementation
const uint32_t RwSpinLock::WRITE_FLAG = 0x80000000;
const uint32_t RwSpinLock::WAIT_WRITE_FLAG = 0x40000000;

bool RwSpinLock::try_once_wlock(uint32_t &spin_count)
{
    uint32_t expected = lock_word_.load(std::memory_order_acquire);
    if ((expected & WAIT_WRITE_FLAG) == 0) {
        // 不能加锁的情况下，需要重新加上预写锁标志，因为有可能多个线程加写锁时，其它线程加上写锁会将预写锁标志清掉
        if (lock_word_.compare_exchange_weak(expected, expected | WAIT_WRITE_FLAG, std::memory_order_acq_rel)) {
            delay(spin_count);
        }
    } else if (expected == WAIT_WRITE_FLAG) {
        // 尝试直接获取写锁
        if (lock_word_.compare_exchange_weak(expected, WRITE_FLAG, std::memory_order_acq_rel)) {
            return true;
        }
    } else {
        delay(spin_count);
    }
    return false;
}

void RwSpinLock::lock() {
    uint32_t spin_count = 0;
    while (!try_once_wlock(spin_count));
}

bool RwSpinLock::try_lock_for(const std::chrono::milliseconds &timeout) {
    auto start = std::chrono::steady_clock::now();
    
    uint32_t spin_count = 0;
    while (true) {
        if (try_once_wlock(spin_count)) {
            return true;
        }
        auto now = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - start);
        if (elapsed >= timeout) {
            // 清除预写锁标志，但是有可能会把其它线程设置的预写标志清除，但是没关系，加锁流程唤醒后会重设预写标志
            lock_word_.fetch_and(~WAIT_WRITE_FLAG, std::memory_order_acq_rel);
            return false;
        }
    }
}

void RwSpinLock::unlock() {
    DB_ASSERT((lock_word_ & WRITE_FLAG) == WRITE_FLAG);
    lock_word_.fetch_and(~WRITE_FLAG, std::memory_order_release);
}

bool RwSpinLock::try_once_rlock(uint32_t &spin_count)
{
    uint32_t current = lock_word_.load(std::memory_order_acquire);
    // 检查是否有写锁或等待的写锁
    if (current & (WRITE_FLAG | WAIT_WRITE_FLAG)) {
        delay(spin_count);
        return false;
    }

    // 尝试增加读计数
    uint32_t new_value = current + 1;
    if (lock_word_.compare_exchange_weak(current, new_value, std::memory_order_acq_rel)) {
        return true;
    }
    return false;
}

void RwSpinLock::lock_shared() {
    
    uint32_t spin_count = 0;
    while (!try_once_rlock(spin_count));
}

bool RwSpinLock::try_lock_shared_for(const std::chrono::milliseconds &timeout) {
    auto start = std::chrono::steady_clock::now();
    uint32_t spin_count = 0;
    while (true) {
        // 检查超时
        if (try_once_rlock(spin_count)) {
            return true;
        }
        auto now = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - start);
        if (elapsed >= timeout) {
            return false;
        }
    }
}

void RwSpinLock::unlock_shared() {
    DB_ASSERT(lock_word_ > 0);
    lock_word_.fetch_sub(1, std::memory_order_release);
}

} // namespace nebula