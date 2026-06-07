#pragma once

#include <memory>
#include <string>
#include <vector>

namespace nebula {

class Arena;

// 内存上下文：管理内存的生命周期(何时释放、属于哪个阶段、属于哪个算子)
// 通过 Arena 进行分配，支持层级子上下文
class MemoryContext {
public:
    // 顶层上下文
    explicit MemoryContext(uint64_t block_size = 4 * 1024 * 1024);
    ~MemoryContext();

    MemoryContext(const MemoryContext &) = delete;
    MemoryContext &operator=(const MemoryContext &) = delete;
    MemoryContext(MemoryContext &&) = delete;
    MemoryContext &operator=(MemoryContext &&) = delete;

    // 创建子上下文(子上下文生命周期由本上下文管理)
    MemoryContext *CreateChild();

    // 从本上下文的 Arena 中分配 size 字节
    void *Allocate(uint64_t size);

    // 逻辑释放：重置 Arena 的偏移，内存可复用
    void Reset();

    // 物理释放：销毁 Arena 并销毁所有子上下文
    void Destroy();

    // 返回本上下文的内存使用量(不含子上下文)
    uint64_t MemoryUsage() const;

    // 返回本上下文及所有子上下文的内存使用量
    uint64_t TotalMemoryUsage() const;

private:
    std::unique_ptr<Arena> arena_;
    std::vector<std::unique_ptr<MemoryContext>> children_;
};

} // namespace nebula
