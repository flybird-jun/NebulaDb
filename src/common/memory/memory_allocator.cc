// 保留 jemalloc 的 je_ 前缀宏，使其在本文件中可用
// (默认情况下 jemalloc 在头文件末尾会 #undef 这些宏)
#define JEMALLOC_NO_DEMANGLE
#include "jemalloc/jemalloc.h"
#undef JEMALLOC_NO_DEMANGLE

#include "db_define.h"
#include "memory_allocator.h"
namespace nebula {
struct AllocHeader {
    void *raw;     // 原始 jemalloc 指针
    uint64_t size;   // 请求的字节数
};

constexpr uint64_t kHeaderSize = sizeof(AllocHeader);

MemoryAllocator &MemoryAllocator::Instance() {
    static MemoryAllocator instance;
    return instance;
}

void *MemoryAllocator::Allocate(uint64_t size) {
    if (size == 0) {
        return nullptr;
    }
    uint64_t total = size + kHeaderSize;
    void *raw = je_malloc(total);
    if (raw == nullptr) {
        return nullptr;
    }
    AllocHeader *hdr = static_cast<AllocHeader *>(raw);
    hdr->raw = raw;
    hdr->size = size;
    void *user = static_cast<char *>(raw) + kHeaderSize;

    uint64_t prev = allocated_bytes_.fetch_add(size, std::memory_order_relaxed);
    UpdatePeak(prev + size);

    return user;
}

void *MemoryAllocator::AllocateAligned(uint64_t size, uint64_t alignment) {
    if (size == 0) {
        return nullptr;
    }
    if (alignment == 0 || (alignment & (alignment - 1)) != 0) {
        return nullptr; // alignment 必须是 2 的幂
    }
    // 为头部 + 对齐填充 + 用户数据预留足够空间
    uint64_t total = size + kHeaderSize + alignment;
    void *raw = je_malloc(total);
    if (raw == nullptr) {
        return nullptr;
    }
    // 计算头部后的对齐地址
    uintptr_t raw_addr = reinterpret_cast<uintptr_t>(raw);
    uintptr_t user_addr = align_up(raw_addr + kHeaderSize, alignment);
    void *user = reinterpret_cast<void *>(user_addr);

    AllocHeader *hdr = reinterpret_cast<AllocHeader *>(user_addr - kHeaderSize);
    hdr->raw = raw;
    hdr->size = size;

    uint64_t prev = allocated_bytes_.fetch_add(size, std::memory_order_relaxed);
    UpdatePeak(prev + size);

    return user;
}

void MemoryAllocator::Deallocate(void *ptr) {
    if (ptr == nullptr) {
        return;
    }
    AllocHeader *hdr = reinterpret_cast<AllocHeader *>(static_cast<char *>(ptr) - kHeaderSize);
    void *raw = hdr->raw;
    uint64_t size = hdr->size;
    je_free(raw);
    allocated_bytes_.fetch_sub(size, std::memory_order_relaxed);
}


uint64_t MemoryAllocator::AllocatedBytes() const {
    return allocated_bytes_.load(std::memory_order_relaxed);
}

uint64_t MemoryAllocator::PeakAllocatedBytes() const {
    return peak_allocated_bytes_.load(std::memory_order_relaxed);
}

void MemoryAllocator::UpdatePeak(uint64_t current) {
    uint64_t peak = peak_allocated_bytes_.load(std::memory_order_relaxed);
    while (current > peak) {
        if (peak_allocated_bytes_.compare_exchange_weak(peak, current,
                                                       std::memory_order_relaxed)) {
            break;
        }
    }
}

} // namespace nebula
