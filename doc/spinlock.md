# 一、概述
本模块实现自旋锁（SpinLock）和读写自旋锁（RwSpinLock），用于热点数据结构的并发保护。
特点：
    1. 完全用户态实现，无内核切换开销。
    2. 支持尝试加锁接口（try_lock/try_rlock/try_wlock）。
    3. 提供 RAII 风格的锁管理，可与 lock_guard、unique_lock、shared_lock结合使用。
    4. NUMA架构友好
可扩展退避策略和统计信息。
# 二、 类设计

class SpinLock {
private:
    struct QNode {
        atomic<uint32_t> lock; // 为什么使用valatile而不是atomic
        QNode *next;
    };
    QNode node_;
    static atomic<QNode*> tail;
public:
    void lock();
    bool try_lock_for(const std::chrono::milliseconds &timeout);
    void unlock();
    
}
class RwSpinLock {
    private:
    struct QNode {
        atomic<uint32_t> locked_; // 为什么使用valatile而不是atomic
        QNode *next;
    };
    QNode node_;
    static atomic<QNode*> tail;
    static atomic<int> readers;
    public:
        void lock();
        bool try_lock_for(const std::chrono::milliseconds &timeout);
        void unlock();
        void lock_shared();
        bool try_lock_shared_for(const std::chrono::milliseconds &timeout);
        void unlock_shared(); 
};

# 三、 原理机制
## 2.1 自旋锁
- 加锁流程
    1. TAS 设置tail为node_, (TAS会返回原tail(记为pTail)的值)
    2. 如果pTail值为空，直接获得锁
    3. 否则，连接队列pTail->next = node_, 并设置locked_ = true, 然后自旋等待locked_ == false
- 解锁流程
    1. 如果node_->next = nullptr, CAS交换tail, nullptr
## 2.2 自旋读写锁
## 2.3 退避策略
- 三阶段退避模型
    step 1: 时间在0 ~ 50 cycles范围内, 使用_mm_pause(), 不让出CPU
    step 2: 时间在50 ~ 500 cycles范围内, std::this_thread::yield() 让出CPU
    step 3: 等待时间大于500 cycles, usleep(1); 睡眠1us
- 本策略适用于自旋锁和自旋读写锁。
# 四、 用例设计
