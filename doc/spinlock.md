# 一、概述
本模块实现自旋锁（SpinLock）和读写自旋锁（RwSpinLock），用于热点数据结构的并发保护。
要求：
    1. 完全用户态实现，无内核切换开销。
    2. 提供 RAII 风格的锁管理，可与 lock_guard、unique_lock、shared_lock结合使用。
    3. 自旋锁实现排队机制，先到先获得锁; 读写自旋锁，实现写锁优先。
可扩展退避策略和统计信息。
# 二、 类设计
class SpinLock {
private:
    atomic<uint32_t> next_tickets_; // 叫号
    atomic<uint32_t> serving_tickets_; // 服务号
public:
    void lock();
    bool try_lock_for(const std::chrono::milliseconds &timeout);
    void unlock();
}
class RwSpinLock {
    private:
        atomic<uint32_t> lock_word_; // 最高位为写锁标志，次高位为等待写锁标志，剩余位是读锁计数
        static const uint32_t WRITE_FLAG;
        static const uint32_t WAIT_WRITE_FLAG;
    public:
        void lock();
        bool try_lock_for(const std::chrono::milliseconds &timeout);
        void unlock();
        void lock_shared();
        bool try_lock_shared_for(const std::chrono::milliseconds &timeout);
        void unlock_shared(); 
};

# 三、 原理机制
## 3.1 自旋锁原理
自旋锁实现方式为票号锁。
- 加锁流程
    1. 通过next_tickets_取号，记为ticket
    2. 如果 serving_tickets_ == ticket，加锁成功
    3. 否则执行退避策略，唤醒后重试第二步、第三步

- 解锁流程
    1. serving_tickets_原子加1

## 3.2 自旋读写锁原理

- 加写锁流程
    1. 如果lock_word_ == 0, 那么CAS(lock_word_, 0, WRITE_FLAG), CAS成功则加锁成功，否则重试加写锁流程
    2. CAS(lock_word_, old_lock_word_, lock_word_ | WAIT_WRITE_FLAG), CAS成功则执行退避策略, 
    3. 退避被唤醒后，重试加写锁流程
- 加读锁流程
    1. 如果 lock_word_ & (WRITE_FLAG|WAIT_WRITE_FLAG) == 0, CAS(lock_word_, old_lock_word, old_lock_word + 1), CAS成功则加锁成功，否则重试加读锁流程
    2. 如果 lock_word_ & (WRITE_FLAG|WAIT_WRITE_FLAG) != 0, 执行退避策略
    3. 退避被唤醒后，重试加读锁流程
- 解写锁流程
    1. CAS(lock_word_, old_lock_word_, old_lock_word_ & ~WRITE_FLAG), 成功则退出，否则重试
- 解读锁流程
    1. CAS(lock_word_, old_lock_word_, old_lock_word_ - 1), 成功则退出，否则重试
## 3.3 退避策略
- 三阶段退避模型
    1. 第一阶段 (0-50 cycles): 使用 `_mm_pause()` 指令, 不让出CPU，减少上下文切换开销, 适用于短时间等待
    2. 第二阶段 (50-500 cycles): 使用 `std::this_thread::yield()`, 让出CPU，给其他线程执行机会, 适用于中等时间等待
    3. 第三阶段 (>500 cycles): 使用 `usleep(1)` 睡眠1微秒, 让出CPU较长时间，避免忙等待, 适用于长时间等待

# 四、 用例设计
## 4.1 SpinLock 测试用例
- 基本加锁解锁
    描述: 单线程加锁后解锁
    步骤:
    1. 创建 SpinLock 实例 lock
    2. lock.lock()
    3. 检查锁状态是否被占用
    4. lock.unlock()
    5. 检查锁状态是否释放
    期望结果:
    - lock.lock() 成功
    - lock.unlock() 后锁可再次加锁

- 多线程 FIFO 排队
    描述: 多线程依次加锁，验证 FIFO 顺序
    步骤:
    1. 创建 SpinLock 实例 lock
    2. 创建 3 个线程 T1, T2, T3
    3. 每个线程执行:
        lock.lock()
        记录获得锁的顺序
        lock.unlock()
    4. 等待所有线程结束
    期望结果:
    - 获得锁的顺序与线程启动顺序一致

- try_lock_for 超时
    描述: 测试加锁超时功能
    步骤:
    1. 创建 SpinLock 实例 lock
    2. 主线程加锁 lock.lock()
    3. 启动一个子线程，尝试 lock.try_lock_for(100ms)
    4. 记录返回结果
    5. 主线程解锁
    期望结果:
    - 子线程 try_lock_for 返回 false（超时）
    - 主线程解锁后，可再次加锁成功

- 退避策略测试
    描述: 高并发下验证退避策略
    步骤:
    1. 创建 SpinLock 实例 lock
    2. 启动 10 个线程，每个线程重复:
        lock.lock()
        延迟 1ms
        lock.unlock()
        记录自旋次数
    3. 汇总自旋统计信息
    期望结果:
    - 自旋次数符合三阶段退避模型
    - 没有线程长时间饥饿
- lock_guard 自动释放锁
    描述:
        验证 SpinLock 可与 std::lock_guard 配合使用
    步骤:
        1. 创建 SpinLock 实例 lock
        2. 进入作用域
        {
            std::lock_guard<SpinLock> guard(lock);
            验证锁已获取
        }
        3. 离开作用域
        4. 再次执行 lock.lock()
    期望结果:
        - lock_guard 构造时成功获取锁
        - 离开作用域自动调用 unlock()
        - 后续 lock.lock() 可以成功获得锁
# 4.2 读写自旋锁
- 单线程读写锁
    描述: 单线程测试读锁和写锁
    步骤:
    1. 创建 RwSpinLock 实例 rwlock
    2. rwlock.lock_shared()
    3. 检查读计数是否为1
    4. rwlock.unlock_shared()
    5. rwlock.lock()
    6. 检查写标志是否被设置
    7. rwlock.unlock()
    期望结果:
    - 读锁/写锁状态正确
    - 解锁后状态恢复为零

- 写锁优先验证
    描述: 多线程读写，验证写锁优先
    步骤:
    1. 创建 RwSpinLock 实例 rwlock
    2. 启动线程 T1: rwlock.lock_shared() 并保持 100ms
    3. 启动线程 T2: rwlock.lock() 并记录开始等待时间
    4. 启动线程 T3: rwlock.lock_shared() 并记录开始等待时间
    5. T1 解锁
    6. T2 获得写锁后解锁
    7. T3 获得读锁
    期望结果:
    - 写线程 T2 优先于 T3 获得锁
    - 读锁数量在写锁期间为0

- try_lock_for / try_lock_shared_for 超时
    描述: 测试加锁超时功能
    步骤:
    1. 主线程加写锁 rwlock.lock()
    2. 子线程尝试 rwlock.try_lock_for(50ms) 和 rwlock.try_lock_shared_for(50ms)
    3. 记录返回结果
    期望结果:
    - 都返回 false（超时）
    - 主线程解锁后，加锁可以成功

- 高并发读写
    描述: 多线程环境验证读写锁正确性
    步骤:
    1. 创建 RwSpinLock 实例 rwlock
    2. 启动 5 个读线程，每个线程重复:
        rwlock.lock_shared()
        延迟 1ms
        rwlock.unlock_shared()
    3. 启动 2 个写线程，每个线程重复:
        rwlock.lock()
        延迟 2ms
        rwlock.unlock()
    4. 记录每次锁状态和获取顺序
    期望结果:
    - 写锁优先于后续读锁
    - 同时读锁数量 <= 读线程数
    - 锁状态在所有操作结束后为0
- unique_lock 写锁模式
    描述:
        验证 unique_lock 与写锁配合使用
    步骤:
        1. 创建 RwSpinLock 实例 rwlock
        2. std::unique_lock<RwSpinLock> ul(rwlock)
        3. 调用 ul.unlock()
        4. 调用 ul.lock()
    期望结果:
        - 支持显式解锁和重新加锁
        - 锁状态正确
- shared_lock 共享读锁
    描述:
        验证多个 shared_lock 可以同时持有读锁
    步骤:
        1. 创建 RwSpinLock 实例 rwlock
        2. 线程T1:
        std::shared_lock<RwSpinLock> sl1(rwlock)
        3. 线程T2:
        std::shared_lock<RwSpinLock> sl2(rwlock)
        4. 同时持有100ms
    期望结果:
        - T1 和 T2 均成功获得读锁
        - 读计数为2
        - 无阻塞
- 写锁优先(shared_lock场景)
描述:
    验证等待写锁时禁止新读者进入
步骤:
    1. T1持有shared_lock
    2. T2请求写锁
       rwlock.lock()
    3. T3请求shared_lock
    4. T1释放读锁
期望结果:
    - T2先获得写锁
    - T3继续等待
    - T2释放后T3获得读锁
验证点:
    WAIT_WRITE_FLAG 生效

# 五、遗留任务
# 六、易错点