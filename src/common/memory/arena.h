#pragma once

#include <cstddef>
#include <vector>

namespace nebula {

// Arena 分配器：bump allocation，从大块内存中切出小块分配
// 通过 MemoryAllocator 申请大块，自己管理块内的偏移
class Arena {
public:
    // block_size: 每个块的大小(字节)，默认 4MB
    explicit Arena(uint64_t block_size = 4 * 1024 * 1024);
    ~Arena();

    Arena(const Arena &) = delete;
    Arena &operator=(const Arena &) = delete;

    // 从 Arena 中分配 size 字节内存(按 8 字节对齐)
    void *Allocate(uint64_t size);

    // 逻辑释放：将所有块的 offset 重置为 0，内存可复用但不归还 OS
    void Reset();

    // 物理释放：将所有块归还给 MemoryAllocator
    void Destroy();

    // 返回当前 Arena 已使用(已分配出)的字节数
    uint64_t MemoryUsage() const;

private:
    struct Block {
        char *data = nullptr;
        uint64_t capacity = 0;
        uint64_t offset = 0;
    };

    // 申请一个新的 block
    void AllocateNewBlock(uint64_t min_size);

    std::vector<Block> blocks_;
    uint64_t block_size_; // 内存块大小
    uint64_t memory_usage_;
};

} // namespace nebula
