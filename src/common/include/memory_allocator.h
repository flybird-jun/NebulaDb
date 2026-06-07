#pragma once

#include <atomic>

namespace nebula {

// 负责统计当前占用内存和历史峰值内存
class MemoryAllocator {
public:
    // 单例接口
    static MemoryAllocator &Instance();

    // 禁止拷贝与移动
    MemoryAllocator(const MemoryAllocator &) = delete;
    MemoryAllocator &operator=(const MemoryAllocator &) = delete;
    MemoryAllocator(MemoryAllocator &&) = delete;
    MemoryAllocator &operator=(MemoryAllocator &&) = delete;

    // 分配 size 字节内存，返回对齐到 sizeof(void*) 的地址
    void *Allocate(uint64_t size);

    // 分配 size 字节内存，按 alignment 对齐
    void *AllocateAligned(uint64_t size, uint64_t alignment);

    // 释放 ptr 指向的内存
    void Deallocate(void *ptr);

    // 当前已分配但未释放的字节数
    uint64_t AllocatedBytes() const;

    // 历史峰值已分配字节数(reset/destroy 不会回退)
    uint64_t PeakAllocatedBytes() const;

private:
    MemoryAllocator() = default;
    ~MemoryAllocator() = default;

    // 更新峰值统计
    void UpdatePeak(uint64_t current);

    std::atomic<uint64_t> allocated_bytes_{0};       // 当前占用内存
    std::atomic<uint64_t> peak_allocated_bytes_{0}; // 历史峰值内存
};

} // namespace nebula
