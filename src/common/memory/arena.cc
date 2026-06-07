#include "db_define.h"
#include "memory_allocator.h"
#include "arena.h"

namespace nebula {
constexpr uint64_t kAlignment = 8;

Arena::Arena(uint64_t block_size)
    : block_size_(block_size == 0 ? 1 : block_size), memory_usage_(0) {}

Arena::~Arena() {
    Destroy();
}

void Arena::AllocateNewBlock(uint64_t min_size) {
    uint64_t cap = block_size_;
    if (min_size > cap) {
        cap = align_up(min_size, kAlignment);
    }
    void *mem = MemoryAllocator::Instance().Allocate(cap);
    if (mem == nullptr) {
        // 分配失败，blocks_ 不变，调用方 Allocate 会返回 nullptr
        return;
    }
    Block blk;
    blk.data = static_cast<char *>(mem);
    blk.capacity = cap;
    blk.offset = 0;
    blocks_.push_back(blk);
}

void *Arena::Allocate(uint64_t size) {
    if (size == 0) {
        return nullptr;
    }
    uint64_t aligned = align_up(size, kAlignment);

    if (blocks_.empty() || blocks_.back().offset + aligned > blocks_.back().capacity) {
        AllocateNewBlock(aligned);
        if (blocks_.empty()) {
            return nullptr;
        }
    }

    Block &blk = blocks_.back();
    void *ptr = blk.data + blk.offset;
    blk.offset += aligned;
    memory_usage_ += aligned;
    return ptr;
}

void Arena::Reset() {
    for (auto &blk : blocks_) {
        blk.offset = 0;
    }
    memory_usage_ = 0;
}

void Arena::Destroy() {
    for (auto &blk : blocks_) {
        if (blk.data != nullptr) {
            MemoryAllocator::Instance().Deallocate(blk.data);
            blk.data = nullptr;
        }
        blk.capacity = 0;
        blk.offset = 0;
    }
    blocks_.clear();
    memory_usage_ = 0;
}

uint64_t Arena::MemoryUsage() const {
    return memory_usage_;
}

} // namespace nebula
