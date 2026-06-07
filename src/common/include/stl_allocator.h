#pragma once
#include "db_define.h"
#include "memory_allocator.h"
#include "memory_context.h"

namespace nebula {

// STL 容器分配器：内存在容器析构时通过 MemoryAllocator 立即释放
// 适用于生命周期与容器一致的局部 STL 容器: 栈上的STL容器
template <typename T>
class StdAllocator {
public:
    using value_type = T;
    StdAllocator() noexcept = default;
    template<typename U>
    StdAllocator(const StdAllocator<U>&) noexcept {}
    T *allocate(size_t n) {
        if (n == 0) {
            return nullptr;
        }

        void *ptr = MemoryAllocator::Instance().Allocate(n * sizeof(T));
        if (ptr == nullptr) {
            throw std::bad_alloc();
        }
        return static_cast<T *>(ptr);
    }

    void deallocate(T *ptr, [[maybe_unused]] size_t n) noexcept {
        if (ptr != nullptr) {
            MemoryAllocator::Instance().Deallocate(ptr);
        }
    }
};

template <typename T, typename U>
bool operator==(const StdAllocator<T> &, const StdAllocator<U> &) noexcept {
    return true;
}

template <typename T, typename U>
bool operator!=(const StdAllocator<T> &, const StdAllocator<U> &) noexcept {
    return false;
}

// STL 容器分配器：内存在 MemoryContext 释放时统一回收
// 适用于生命周期由 MemoryContext 管理的 STL 容器(如 Query/Operator 内的临时容器)
// deallocate 为 no-op，物理内存随 MemoryContext::Reset/Destroy 统一释放
template <typename T>
class LocalAllocator {
public:
    using value_type = T;
    explicit LocalAllocator(MemoryContext *ctx) noexcept : ctx_(ctx) {
        DB_ASSERT(ctx_ != nullptr);
    }
    template<typename U>
    LocalAllocator(const LocalAllocator<U>& other) noexcept {
        ctx_ = other.GetContext();
    }
    T *allocate(size_t n) {
        if (n == 0) {
            return nullptr;
        }
        void *ptr = ctx_->Allocate(n * sizeof(T));
        if (ptr == nullptr) {
            throw std::bad_alloc();
        }
        return static_cast<T *>(ptr);
    }

    void deallocate(T *, size_t ) noexcept {
    }
    MemoryContext* GetContext() const noexcept {
        return ctx_;
    }
private:
    MemoryContext *ctx_;
};

template <typename T, typename U>
bool operator==(const LocalAllocator<T> &a, const LocalAllocator<U> &b) noexcept {
    return a.GetContext() == b.GetContext();
}

template <typename T, typename U>
bool operator!=(const LocalAllocator<T> &a, const LocalAllocator<U> &b) noexcept {
    return a.GetContext() != b.GetContext();
}

} // namespace nebula
