#include <memory>
#include "memory_context.h"
#include "arena.h"
namespace nebula {

MemoryContext::MemoryContext(uint64_t block_size)
{
    arena_ = std::make_unique<Arena>(block_size);
}

MemoryContext::~MemoryContext() {
    // unique_ptr 会自动销毁 arena_ 和 children_
}

MemoryContext *MemoryContext::CreateChild() {
    auto child = std::make_unique<MemoryContext>();
    MemoryContext *raw = child.get();
    children_.push_back(std::move(child));
    return raw;
}

void *MemoryContext::Allocate(uint64_t size) {
    if (arena_ == nullptr) {
        return nullptr;
    }
    return arena_->Allocate(size);
}

void MemoryContext::Reset() {
    if (arena_ != nullptr) {
        arena_->Reset();
    }
    for (auto &child : children_) {
        if (child) {
            child->Reset();
        }
    }
}

void MemoryContext::Destroy() {
    if (arena_ != nullptr) {
        arena_->Destroy();
    }
    children_.clear();
}

uint64_t MemoryContext::MemoryUsage() const {
    return arena_ != nullptr ? arena_->MemoryUsage() : 0;
}

uint64_t MemoryContext::TotalMemoryUsage() const {
    uint64_t total = MemoryUsage();
    for (const auto &child : children_) {
        if (child) {
            total += child->TotalMemoryUsage();
        }
    }
    return total;
}

} // namespace nebula
