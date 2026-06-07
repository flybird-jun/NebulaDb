# 一、概述
# 二、类设计
```cpp
class WalFile {

}
class WalBuf {
public:
    WalBuf(std::shared_ptr<MemoryContext> &mem_ctx);
private:
    vector<char, LocalAllocator<char>> 
}
class WalCtx {
public:
    WalCtx(std::shared_ptr<MemoryContext> &mem_ctx, uint32_t buf_size):data(LocalAllocator<char>(mem_ctx));
    Status mini_trx_start();
    void record(uint32_t type, PageIdT page_id, std::vector<char> &data);
    Status mini_trx_end();
    ~WalCtx();
private:
    std::vector<char, LocalAllocator<char>> data;
}
class WalMgr {
private:
    bool init_;
    WalBuf wal_buf_;
    std::shared_ptr<MemoryContext> mem_ctx_;

public:
    static std::shared_ptr<WalMgr> instance();
    void Init();
    std::unique_ptr<WalCtx> CreateWalCtx(std::shared_ptr<MemoryContext> &mem_ctx);
};

class WalInterface {
public:
    enum WAL_TYPE {
        WAL_TYPE_ALLOC_PAGE
    };
    static Status mini_trx_start(WalCtx *ctx);
    static void record(WAL_TYPE type, PageIdT page_id, std::vector<char> &data);
    static Status mini_trx_end(WalCtx *ctx);
    static Status wal_flush();
    static std::unique_ptr<WalCtx> CreateWalCtx();
}
```
# 三、原理
## 3.1 架构设计
- 文件设计
storage/
    |
    |___ include/
    |       |
    |       |_____ wal_interface.h
    |
    |___ wal/
    |     |
    |     |_____ wal_buf.cc
    |     |
    |     |_____ wal_mgr.cc
    |     |
    |     |_____ wal_file.cc
    |
    |___ wal_interface.cc
# 四、用例设计