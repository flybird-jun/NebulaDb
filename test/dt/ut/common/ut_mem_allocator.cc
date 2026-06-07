#include <gtest/gtest.h>
#include <atomic>
#include <thread>
#include <vector>
#include <string>
#include <random>
#include <cstring>

#include "memory_allocator.h"
#include "memory_context.h"
#include "stl_allocator.h"

using namespace nebula;

// ============== 4.1 MemoryAllocator 用例 ==============

// 基本内存分配与释放
TEST(MemoryAllocatorTest, BasicAllocFree) {
    void *ptr = MemoryAllocator::Instance().Allocate(128);
    EXPECT_NE(ptr, nullptr);
    // 写入以验证地址可写
    std::memset(ptr, 0xAB, 128);
    MemoryAllocator::Instance().Deallocate(ptr);
}

// 峰值内存统计
TEST(MemoryAllocatorTest, PeakMemoryStats) {
    auto &ma = MemoryAllocator::Instance();
    const uint64_t before = ma.AllocatedBytes();

    // 分配 1MB x 10 次
    constexpr uint64_t kChunk = 1 * 1024 * 1024;
    std::vector<void *> ptrs;
    ptrs.reserve(10);
    for (int i = 0; i < 10; ++i) {
        void *p = ma.Allocate(kChunk);
        ASSERT_NE(p, nullptr);
        ptrs.push_back(p);
    }
    const uint64_t after_alloc = ma.AllocatedBytes();
    EXPECT_GE(after_alloc, before + 10 * kChunk);
    const uint64_t peak_after_alloc = ma.PeakAllocatedBytes();
    // 峰值应至少达到本次分配的总量
    EXPECT_GE(peak_after_alloc, 10 * kChunk);

    // 释放一半
    for (int i = 0; i < 5; ++i) {
        ma.Deallocate(ptrs[i]);
    }
    ptrs.erase(ptrs.begin(), ptrs.begin() + 5);

    // 当前内存下降
    EXPECT_LT(ma.AllocatedBytes(), after_alloc);
    // 峰值保持不变
    EXPECT_EQ(ma.PeakAllocatedBytes(), peak_after_alloc);

    // 释放剩余
    for (void *p : ptrs) {
        ma.Deallocate(p);
    }
}

// 多线程并发分配
TEST(MemoryAllocatorTest, ConcurrentAlloc) {
    auto &ma = MemoryAllocator::Instance();
    const int thread_count = 10;
    const int per_thread = 1000;
    std::vector<std::thread> threads;
    std::vector<std::vector<void *>> all_ptrs(thread_count);

    for (int t = 0; t < thread_count; ++t) {
        threads.emplace_back([t, &ma, &all_ptrs]() {
            auto &my = all_ptrs[t];
            my.reserve(per_thread);
            for (int i = 0; i < per_thread; ++i) {
                void *p = ma.Allocate(64 + (i % 32));
                EXPECT_NE(p, nullptr);
                my.push_back(p);
            }
        });
    }
    for (auto &th : threads) th.join();

    // 释放所有
    for (auto &vec : all_ptrs) {
        for (void *p : vec) {
            ma.Deallocate(p);
        }
    }
    // 不崩溃即通过
    EXPECT_GE(ma.AllocatedBytes(), 0u);
}
// ============== 4.3 MemoryContext 用例 ==============

// 单层 Context 分配
TEST(MemoryContextTest, SingleLevelAlloc) {
    auto &ma = MemoryAllocator::Instance();
    uint64_t before = ma.AllocatedBytes();

    {
        MemoryContext ctx;
        void *p1 = ctx.Allocate(1024);
        void *p2 = ctx.Allocate(2048);
        EXPECT_NE(p1, nullptr);
        EXPECT_NE(p2, nullptr);
        EXPECT_GT(ctx.MemoryUsage(), 0u);
    }

    // 析构后内存归还
    EXPECT_LE(ma.AllocatedBytes(), before);
}

// 层级 Context 生命周期
TEST(MemoryContextTest, HierarchicalLifecycle) {
    MemoryContext root;
    MemoryContext *child = root.CreateChild();
    ASSERT_NE(child, nullptr);

    void *p = child->Allocate(4096);
    EXPECT_NE(p, nullptr);
    EXPECT_GT(child->MemoryUsage(), 0u);

    // child Destroy 后 arena 释放, 内存使用量归零
    child->Destroy();
    EXPECT_EQ(child->MemoryUsage(), 0u);
    // root 不受影响
    EXPECT_GE(root.MemoryUsage(), 0u);
}

// Reset 行为验证
TEST(MemoryContextTest, ResetReuse) {
    MemoryContext ctx;
    ctx.Allocate(1024);
    ctx.Allocate(2048);
    uint64_t usage_before = ctx.MemoryUsage();
    EXPECT_GT(usage_before, 0u);

    ctx.Reset();
    EXPECT_EQ(ctx.MemoryUsage(), 0u);

    // 再次分配同样大小, 复用旧 block
    ctx.Allocate(1024);
    EXPECT_EQ(ctx.MemoryUsage(), 1024u);
    (void)usage_before;
}

// Destroy vs Reset 对比
TEST(MemoryContextTest, DestroyVsReset) {
    auto &ma = MemoryAllocator::Instance();

    // Reset 场景
    uint64_t before_reset = ma.AllocatedBytes();
    {
        MemoryContext ctx;
        ctx.Allocate(2 * 1024 * 1024);
        ctx.Reset();
        // Reset 后 Arena 偏移归零，但块仍在
        // 注意: 重新分配会复用旧块
        ctx.Allocate(1024);
    }
    // 析构后归还
    EXPECT_LE(ma.AllocatedBytes(), before_reset);

    // Destroy 场景
    uint64_t before_destroy = ma.AllocatedBytes();
    {
        MemoryContext ctx;
        ctx.Allocate(2 * 1024 * 1024);
        ctx.Destroy();
        EXPECT_EQ(ctx.MemoryUsage(), 0u);
    }
    EXPECT_LE(ma.AllocatedBytes(), before_destroy);
}

// 多子 Context 管理
TEST(MemoryContextTest, MultiChildManagement) {
    auto &ma = MemoryAllocator::Instance();
    uint64_t before = ma.AllocatedBytes();

    {
        MemoryContext root;
        for (int i = 0; i < 5; ++i) {
            MemoryContext *child = root.CreateChild();
            child->Allocate(1024 * (i + 1));
        }
        // TotalMemoryUsage 应能累加所有子 context 的内存
        EXPECT_GT(root.TotalMemoryUsage(), 0u);
    }
    // 析构后所有 child 自动释放
    EXPECT_LE(ma.AllocatedBytes(), before);
}

// ============== 4.4 StdAllocator 用例 ==============

TEST(StdAllocatorTest, VectorBasicAlloc) {
    std::vector<int, StdAllocator<int>> vec;
    for (int i = 0; i < 100; ++i) {
        vec.push_back(i);
    }
    EXPECT_EQ(vec.size(), 100u);
    for (int i = 0; i < 100; ++i) {
        EXPECT_EQ(vec[i], i);
    }
}

TEST(StdAllocatorTest, VectorRealloc) {
    std::vector<int, StdAllocator<int>> vec;
    // 触发多次扩容
    for (int i = 0; i < 10000; ++i) {
        vec.push_back(i);
    }
    EXPECT_EQ(vec.size(), 10000u);
    EXPECT_EQ(vec.back(), 9999);
}

TEST(StdAllocatorTest, StringAlloc) {
    std::basic_string<char, std::char_traits<char>, StdAllocator<char>> s;
    s = "hello world";
    EXPECT_EQ(s, "hello world");
}

// ============== 4.5 LocalAllocator 用例 ==============

TEST(LocalAllocatorTest, ContextBoundAlloc) {
    auto &ma = MemoryAllocator::Instance();
    uint64_t before = ma.AllocatedBytes();

    MemoryContext ctx;
    {
        std::vector<int, LocalAllocator<int>> vec{LocalAllocator<int>(&ctx)};
        for (int i = 0; i < 100; ++i) {
            vec.push_back(i);
        }
        EXPECT_EQ(vec.size(), 100u);
    }
    // vector 析构后，LocalAllocator::deallocate 是 no-op
    // 内存仍由 ctx 管理
    uint64_t after_vec_dtor = ma.AllocatedBytes();
    EXPECT_GE(after_vec_dtor, before); // 内存仍在使用

    // ctx.Reset 释放所有内存
    ctx.Reset();
    EXPECT_EQ(ctx.MemoryUsage(), 0u);
}

TEST(LocalAllocatorTest, DeallocateNoOp) {
    MemoryContext ctx;
    void *p = ctx.Allocate(1024);
    EXPECT_NE(p, nullptr);
    uint64_t usage_before = ctx.MemoryUsage();

    // vector 析构时 LocalAllocator::deallocate 应为 no-op,
    // 即 vector 使用的内存不会被释放, ctx 内存使用量不变
    uint64_t usage_after_alloc;
    {
        std::vector<int, LocalAllocator<int>> vec{LocalAllocator<int>(&ctx)};
        vec.push_back(1);
        usage_after_alloc = ctx.MemoryUsage();
        EXPECT_GT(usage_after_alloc, usage_before);
    }
    // vector 析构后, ctx 内存使用量应保持不变(no-op dealloc)
    EXPECT_EQ(ctx.MemoryUsage(), usage_after_alloc);
    (void)p;
}

TEST(LocalAllocatorTest, ContextResetBatchFree) {
    MemoryContext ctx;
    {
        std::vector<int, LocalAllocator<int>> vec{LocalAllocator<int>(&ctx)};
        for (int i = 0; i < 1000; ++i) {
            vec.push_back(i);
        }
    }
    uint64_t usage_before = ctx.MemoryUsage();
    EXPECT_GT(usage_before, 0u);

    ctx.Reset();
    EXPECT_EQ(ctx.MemoryUsage(), 0u);
}

TEST(LocalAllocatorTest, ContextDestroyFree) {
    auto &ma = MemoryAllocator::Instance();
    uint64_t before = ma.AllocatedBytes();

    {
        MemoryContext ctx;
        std::vector<int, LocalAllocator<int>> vec{LocalAllocator<int>(&ctx)};
        for (int i = 0; i < 1000; ++i) {
            vec.push_back(i);
        }
    }
    EXPECT_LE(ma.AllocatedBytes(), before);
}

// ============== 4.7 异常与边界用例 ==============

TEST(MemoryContextTest, DoubleDestroy) {
    MemoryContext ctx;
    ctx.Allocate(1024);
    ctx.Destroy();
    // 第二次 Destroy 不应崩溃
    EXPECT_NO_THROW(ctx.Destroy());
}

TEST(MemoryAllocatorTest, DeallocateNull) {
    auto &ma = MemoryAllocator::Instance();
    EXPECT_NO_THROW(ma.Deallocate(nullptr));
}

TEST(MemoryAllocatorTest, ZeroSizeAlloc) {
    auto &ma = MemoryAllocator::Instance();
    void *p = ma.Allocate(0);
    // 0 字节分配返回 nullptr 是合理行为
    EXPECT_EQ(p, nullptr);
}

TEST(MemoryAllocatorTest, AlignedAlloc) {
    auto &ma = MemoryAllocator::Instance();
    void *p = ma.AllocateAligned(1024, 64);
    EXPECT_NE(p, nullptr);
    EXPECT_EQ(reinterpret_cast<uintptr_t>(p) % 64, 0u);
    ma.Deallocate(p);
}
