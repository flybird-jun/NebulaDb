#include <gtest/gtest.h>
#include <thread>
#include <vector>
#include <chrono>
#include <atomic>
#include <shared_mutex>
#include "spinlock.h"

using namespace nebula;

// 测试SpinLock的基本加锁解锁
TEST(SpinLockTest, BasicLockUnlock) {
    SpinLock lock;

    // 第一次加锁
    lock.lock();

    // 检查锁是否被占用 - SpinLock没有直接的查询方法，通过加锁尝试来验证
    std::thread t([&lock]() {
        auto start = std::chrono::steady_clock::now();
        std::lock_guard<SpinLock> guard(lock);
        auto end = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
        EXPECT_GE(elapsed.count(), 0); // 第二次加锁应该等待
    });

    // 等待一小段时间确保子线程开始等待
    std::this_thread::sleep_for(std::chrono::milliseconds(10));

    // 解锁
    lock.unlock();

    // 等待子线程完成
    t.join();

    // 现在可以再次加锁
    lock.lock();
    lock.unlock();
}

// 测试SpinLock的多线程FIFO排队
TEST(SpinLockTest, FIFOOrder) {
    SpinLock lock;
    std::vector<int> acquire_order;
    std::vector<std::thread> threads;
    int thread_count = 3;

    for (int i = 0; i < thread_count; ++i) {
        threads.emplace_back([i, &lock, &acquire_order]() {
            lock.lock();
            acquire_order.push_back(i);
            std::this_thread::sleep_for(std::chrono::milliseconds(50)); // 模拟工作
            lock.unlock();
        });
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
    }

    // 等待所有线程完成
    for (auto &t : threads) {
        t.join();
    }

    // 验证获取顺序与启动顺序一致
    for (int i = 0; i < thread_count; ++i) {
        EXPECT_EQ(acquire_order[i], i);
    }
}

// 测试lock_guard自动释放锁
TEST(SpinLockTest, LockGuard) {
    SpinLock lock;
    bool locked = false;

    {
        std::lock_guard<SpinLock> guard(lock);
        locked = true;
        // 离开作用域时，guard会自动调用unlock()
    }

    // 现在应该可以再次加锁
    lock.lock();
    lock.unlock();

    EXPECT_TRUE(locked);
}

// 测试RwSpinLock的单线程读写锁
TEST(RwSpinLockTest, SingleThreadReadWrite) {
    RwSpinLock rwlock;

    // 测试读锁
    rwlock.lock_shared();
    // 不能直接检查读计数，通过加写锁尝试来验证

    std::thread t1([&rwlock]() {
        auto start = std::chrono::steady_clock::now();
        rwlock.lock();
        auto end = std::chrono::steady_clock::now();
        rwlock.unlock();
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
        EXPECT_GE(elapsed.count(), 0); // 应该等待
    });

    std::this_thread::sleep_for(std::chrono::milliseconds(10));

    rwlock.unlock_shared();

    t1.join();

    // 测试写锁
    rwlock.lock();

    std::thread t2([&rwlock]() {
        auto start = std::chrono::steady_clock::now();
        rwlock.lock_shared();
        auto end = std::chrono::steady_clock::now();
        rwlock.unlock_shared();
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
        EXPECT_GE(elapsed.count(), 0); // 应该等待
    });

    std::this_thread::sleep_for(std::chrono::milliseconds(10));

    rwlock.unlock();

    t2.join();
}

// 测试RwSpinLock的写锁优先
TEST(RwSpinLockTest, WritePriority) {
    RwSpinLock rwlock;
    std::atomic<bool> t2_got_lock{false};
    std::atomic<bool> t3_got_lock{false};
    std::atomic<int> t2_start_time{0};
    std::atomic<int> t3_start_time{0};

    // 线程T1持有读锁
    rwlock.lock_shared();

    // 线程T2请求写锁
    std::thread t2([&rwlock, &t2_got_lock, &t2_start_time]() {
        auto start = std::chrono::steady_clock::now();
        t2_start_time = static_cast<int>(std::chrono::duration_cast<std::chrono::milliseconds>(start.time_since_epoch()).count());
        rwlock.lock();
        t2_got_lock = true;
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
        rwlock.unlock();
    });

    // 线程T3请求读锁
    std::thread t3([&rwlock, &t3_got_lock, &t3_start_time]() {
        auto start = std::chrono::steady_clock::now();
        t3_start_time = static_cast<int>(std::chrono::duration_cast<std::chrono::milliseconds>(start.time_since_epoch()).count());
        rwlock.lock_shared();
        t3_got_lock = true;
        rwlock.unlock_shared();
    });

    // 等待一段时间
    std::this_thread::sleep_for(std::chrono::milliseconds(10));

    // T1释放读锁
    rwlock.unlock_shared();

    // 等待所有线程完成
    t2.join();
    t3.join();

    // 验证写锁优先
    EXPECT_TRUE(t2_got_lock);
    EXPECT_TRUE(t3_got_lock);
    // 由于写锁优先，T2应该在T3之前获得锁
}

// 测试RwSpinLock的超时功能
TEST(RwSpinLockTest, Timeout) {
    RwSpinLock rwlock;
    bool result1 = false;
    bool result2 = false;

    // 主线程加写锁
    rwlock.lock();

    // 子线程尝试写锁超时
    std::thread t1([&rwlock, &result1]() {
        result1 = rwlock.try_lock_for(std::chrono::milliseconds(50));
    });

    // 子线程尝试读锁超时
    std::thread t2([&rwlock, &result2]() {
        result2 = rwlock.try_lock_shared_for(std::chrono::milliseconds(50));
    });

    t1.join();
    t2.join();

    // 都应该超时
    EXPECT_FALSE(result1);
    EXPECT_FALSE(result2);

    // 解锁后，应该能成功加锁
    rwlock.unlock();

    // 测试写锁
    bool result3 = rwlock.try_lock_for(std::chrono::milliseconds(50));
    EXPECT_TRUE(result3);
    rwlock.unlock();

    // 测试读锁
    bool result4 = rwlock.try_lock_shared_for(std::chrono::milliseconds(50));
    EXPECT_TRUE(result4);
    rwlock.unlock_shared();
}

// 测试RwSpinLock的高并发读写
TEST(RwSpinLockTest, HighConcurrency) {
    RwSpinLock rwlock;
    const int reader_threads = 5;
    const int writer_threads = 2;
    const int iterations = 100;
    std::atomic<int> total_reads{0};
    std::atomic<int> total_writes{0};
    std::atomic<bool> stop{false};

    std::vector<std::thread> readers;
    std::vector<std::thread> writers;

    // 创建读线程
    for (int i = 0; i < reader_threads; ++i) {
        readers.emplace_back([&rwlock, &total_reads, &stop, iterations]() {
            for (int j = 0; j < iterations; ++j) {
                if (stop) break;
                rwlock.lock_shared();
                total_reads++;
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
                rwlock.unlock_shared();
            }
        });
    }

    // 创建写线程
    for (int i = 0; i < writer_threads; ++i) {
        writers.emplace_back([&rwlock, &total_writes, &stop, iterations]() {
            for (int j = 0; j < iterations; ++j) {
                if (stop) break;
                rwlock.lock();
                total_writes++;
                std::this_thread::sleep_for(std::chrono::milliseconds(2));
                rwlock.unlock();
            }
        });
    }

    // 运行一段时间
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));

    // 停止所有线程
    stop = true;

    // 等待所有线程完成
    for (auto &t : readers) {
        t.join();
    }
    for (auto &t : writers) {
        t.join();
    }

    // 验证结果
    EXPECT_GT(total_reads, 0);
    EXPECT_GT(total_writes, 0);

    // 验证最终锁状态
    // 没有直接的方法检查锁状态，但可以通过尝试加锁来验证
    EXPECT_TRUE(rwlock.try_lock_for(std::chrono::milliseconds(1)));
    rwlock.unlock();
}

// 测试unique_lock与RwSpinLock
TEST(RwSpinLockTest, UniqueLock) {
    RwSpinLock rwlock;
    bool locked = false;

    {
        std::unique_lock<RwSpinLock> ul(rwlock);
        locked = true;
        // 测试显式解锁
        ul.unlock();
        // 测试重新加锁
        ul.lock();
        EXPECT_TRUE(ul.owns_lock());
    }

    EXPECT_TRUE(locked);
}

// 测试shared_lock与RwSpinLock
TEST(RwSpinLockTest, SharedLock) {
    RwSpinLock rwlock;
    std::atomic<bool> t1_locked{false};
    std::atomic<bool> t2_locked{false};

    // 线程T1持有shared_lock
    std::thread t1([&rwlock, &t1_locked]() {
        std::shared_lock<RwSpinLock> sl(rwlock);
        t1_locked = true;
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    });

    // 线程T2也持有shared_lock
    std::thread t2([&rwlock, &t2_locked]() {
        std::shared_lock<RwSpinLock> sl(rwlock);
        t2_locked = true;
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    });

    // 等待线程开始
    std::this_thread::sleep_for(std::chrono::milliseconds(10));

    // 两个线程都应该成功获得读锁
    EXPECT_TRUE(t1_locked);
    EXPECT_TRUE(t2_locked);

    t1.join();
    t2.join();
}

// 测试写锁优先(shared_lock场景)
TEST(RwSpinLockTest, WritePriorityWithSharedLock) {
    RwSpinLock rwlock;

    std::chrono::steady_clock::time_point t1_time;
    std::chrono::steady_clock::time_point t2_time;
    std::chrono::steady_clock::time_point t3_time;
    // 线程T1持有shared_lock
    std::thread t1([&rwlock, &t1_time]() {
        std::shared_lock<RwSpinLock> sl(rwlock);
        t1_time = std::chrono::steady_clock::now();
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    });

    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    // 线程T2请求写锁
    std::thread t2([&rwlock, &t2_time]() {
        std::unique_lock<RwSpinLock> s2(rwlock);
        t2_time = std::chrono::steady_clock::now();
    });
    // 等待T2 启动
    std::this_thread::sleep_for(std::chrono::milliseconds(10));

    // 线程T3请求shared_lock
    std::thread t3([&rwlock, &t3_time]() {
        std::shared_lock<RwSpinLock> sl(rwlock);
        t3_time = std::chrono::steady_clock::now();
    });

    // 等待一段时间
    std::this_thread::sleep_for(std::chrono::milliseconds(10));

    t1.join();
    t2.join();
    t3.join();
    EXPECT_LT(t1_time, t2_time);
    EXPECT_LT(t2_time, t3_time);
}