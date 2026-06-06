# 一、概述
## 1.1 背景
    数据库内核在运行过程中需要记录各类事件信息，包括正常运行状态、错误信息、调试信息、性能指标等。一个良好的日志模块对于数据库的故障诊
断、性能调优、运行监控至关重要。
## 1.2 目标
    设计一个同步、低延迟、低侵入的日志打印模块，满足数据库内核在各类环境下的日志记录需求。简化实现，避免异步带来的线程管理和内存队列复
杂性，保证日志的可靠性（每条日志调用返回时确保已写入输出目标）。
## 1.3 功能需求
    - 支持多个日志级别：DEBUG、INFO、ERROR
    - 支持日志模块分类: Storage、Executor、Common、Query等
    - 支持日志文件自动轮转
    - 同步日志写入 – 每条日志调用返回时保证已提交到操作系统
    - 日志单条写入不超过10us
    - 日志格式为: time [MODULE] pid tid | fmt-string
      其中 time格式为 YY-MM-DD HH:MM:SS.ms，精确到ms
# 二、类设计
// 单例模式, 需添加禁止拷贝、赋值、移动构造函数
class Logger {
public:
    enum Level {
        DEBUG,
        INFO,
        ERROR
    };
    static Logger& get_logger();
    void init(string path, Level level, int file_count, int capacity);
    void write(Level level, const string& module, const char *format, ...);
    ~Logger(); // 析构前需要flush
private:
    Logger() = default;
    string path_; // 目录
    string file_name_; // 文件名前缀，文件全路径为path_ + file_name_ + "." + cur_file_no_ + ".log"
    int file_count_; // 文件最大数量
    int cur_file_hwm_; // 当前写到的文件号的最大值
    int file_capacity_; // 每个文件最多可以占用的磁盘空间，单位为KB
    pid_t pid;
    Level effect_level_;
    mutex mutex_; // 保护日志文件轮转时fd的并发
    int fd;
};
# 三、方案原理
## 3.1 日志文件轮转
    写入的文件名一直是path_ + file_name_ + "." + ".log", 写满时，将文件名重命名为path_ + file_name_ + ".1" + ".log", 如果已经存在
日志文件，依次重命名。举例如下：
    假设log目录下有文件：
        test.log、test.1.log、test.2.log、test.3.log
    现在test.log文件大小超过capacity, 需要进行重命名:
                      test.log -- 新增文件，可以写入新的日志
    test.log(old) --> test.l.log
    test.1.log(old) --> test.2.log
    test.2.log(old) --> test.3.log
    test.3.log(old) --> test.4.log
    如果文件个数已经达到最大值，那么最后一个日志文件丢弃删除，比如说上面的例子，如果最大文件个数为4，那么就要丢弃test.4.log
## 3.2 写文件并发设计
   使用linux文件IO接口，当使用O_APPEND方式打开文件时，调用write接口，操作系统可以保证追加写不会出现并发问题，因此不需要加锁
# 四、用例设计
## 4.1 日志级别过滤
用例名称：日志级别过滤 - 仅输出高于或等于当前级别的日志
前置条件：日志模块初始化，设置有效级别为 INFO；日志路径可写；文件轮转参数：容量 1KB，最大文件数 3
测试步骤：
依次调用 write(DEBUG, "Common", "debug msg")
调用 write(INFO, "Common", "info msg")
调用 write(ERROR, "Common", "error msg")
预期结果：日志文件中只包含 "info msg" 和 "error msg"，不包含 "debug msg"
## 4.2 日志模块标签
用例名称：日志模块标签 - 正确显示[MODULE]
前置条件：日志级别为 DEBUG
测试步骤：分别使用模块 "Storage", "Executor", "Common", "Query" 调用 write(INFO, ...)
预期结果：每条日志的格式中包含对应的模块名，例如 [Storage]、[Executor] 等
## 4.3 日志格式
用例名称：日志格式 - 时间、PID、TID、消息体符合规范
前置条件：日志级别为 DEBUG
测试步骤：
调用 write(INFO, "Common", "test message %d", 123)
读取最新日志行
预期结果：日志行格式如：25-01-15 14:30:45.123 [Common] 12345 67890 | test message 123
其中时间为 YY-MM-DD HH:MM:SS.ms，PID/TID 为整数
## 4.4 支持 printf 风格格式化
用例名称：支持 printf 风格格式化
前置条件：级别为 DEBUG
测试步骤：调用 write(INFO, "Storage", "block %d size %zu", 10, 4096)
预期结果：日志消息正确解析为 "block 10 size 4096"

## 4.5 日志轮转测试
用例名称：文件容量达到上限时自动轮转
前置条件：设置容量为 1KB，最大文件数 3；日志目录初始为空
测试步骤：
写入 0.9KB 日志（约 900 字节） → 生成 test.log
写入 0.2KB 日志（触发超限）
预期结果：
原始 test.log 被重命名为 test.1.log
新建 test.log 并写入新日志
test.1.log 内容为之前的 0.9KB 数据
## 4.6 多次轮转时重命名滑动正确
用例名称：多次轮转时重命名滑动正确
前置条件：容量 1KB，最大文件数 3；目录下已存在 test.1.log, test.2.log
测试步骤：写入触发轮转的日志
预期结果：
test.log → test.1.log
原 test.1.log → test.2.log
原 test.2.log → test.3.log
新建 test.log
## 4.7 文件数量达到上限时删除最旧文件
用例名称：文件数量达到上限时删除最旧文件
前置条件：容量 1KB，最大文件数 3；目录下已有 test.1.log, test.2.log, test.3.log
测试步骤：触发一次轮转
预期结果：
test.log → test.1.log
原 test.1.log → test.2.log
原 test.2.log → test.3.log
原 test.3.log 被删除
新建 test.log
## 4.8 文件编号异常测试
用例名称：缺失中间编号文件时仍能正确轮转
前置条件：目录下只有 test.log 和 test.3.log（缺 1、2）
测试步骤：触发轮转
预期结果：
test.log → test.1.log
test.3.log 保持不变（不要求连续编号，但原有文件不被破坏）
新建 test.log

## 4.9 并发与同步测试-01
用例名称：多线程同时写日志，单条日志内容完整不被打断
前置条件：日志级别 DEBUG；启用 O_APPEND
测试步骤：启动 10 个线程，每个线程写 1000 条固定格式日志（如 "thread X seq Y"），所有线程同时运行
预期结果：
日志总行数 = 10000
每行日志格式正确，无字符交错或断裂
每行内容中的 thread id 和序号与写入一致
## 4.10 并发与同步测试-02
用例名称：所有 write 调用返回后日志均已写入 OS 缓冲区
前置条件：模拟正常负载
测试步骤：多线程共写入 N 条日志，待所有线程结束后，检查文件行数
预期结果：文件行数 = N，无丢失
## 4.11 Logger析构测试
用例名称：Logger 对象析构前 flush 所有未写入数据
前置条件：禁用文件系统的自动刷盘（仅测试逻辑）
测试步骤：
写入一批日志，不主动 flush
销毁 Logger 对象
检查文件内容
预期结果：所有日志均已写入文件（即使文件未关闭，但 OS 缓冲区已刷入，或至少 close 前调用了 flush）

## 4.12 性能测试-01
用例名称：单条日志（短消息，约64字节）写入时间满足要求（平均 ≤10us）
前置条件：高性能测试环境，CPU 不降频，无磁盘 IO 瓶颈；日志模块已 warmup
测试步骤：
循环调用 write(INFO, "Common", "short log") 10000 次
测量每次调用的耗时（使用高精度时钟）
统计平均耗时、P99、P999
预期结果：平均耗时 ≤10us，P99 ≤10us（允许极少数抖动）
## 4.13 性能测试-02
用例名称：写入 1KB 日志的耗时 ≤20us
前置条件：同性能测试环境
测试步骤：生成 1KB 消息（含格式化），写入 1000 次，测量耗时
预期结果：平均耗时 ≤20us（可酌情放宽，但仍需远小于异步模型开销）

## 4.14 边界与异常测试-01
用例名称：空消息
测试步骤：调用 write(INFO, "Common", "") 和 write(INFO, "Common", NULL)（若允许）
预期结果：日志输出仅包含时间、模块、PID/TID，消息体为空；程序不崩溃
## 4.15 边界与异常测试-02
用例名称：超长消息（10KB）
测试步骤：构造 10KB 字符串，调用 write
预期结果：日志完整写入，不截断，不丢失结尾
## 4.16 边界与异常测试-02
用例名称：消息中包含换行、回车、制表符等特殊字符
测试步骤：write(INFO, "Common", "line1\nline2")
预期结果：日志文件中出现换行，但单条日志记录本身仍占据一行（允许消息内换行，但不破坏日志行的头部/尾部格式）
## 4.17 边界与异常测试-03
用例名称：磁盘空间不足
前置条件：使用测试专用小分区或用 fallocate 填满
测试步骤：触发日志写入
预期结果：write 返回错误，但模块不应崩溃；后续调用应能处理错误（尝试重命名轮转失败，继续报错或忽略）；应有错误日志输出到 stderr 或预定义错误位置
## 4.18 边界与异常测试-04
用例名称：日志目录不存在或权限不足
前置条件：设置日志路径为 /nonexist/
测试步骤：创建 Logger 对象
预期结果：构造失败或抛出异常；程序可捕获并处理

## 4.19 可靠性测试-01
用例名称：崩溃前已 write 返回的日志应存在于文件中
前置条件：使用 O_APPEND，不调用 fsync
测试步骤：写入一条日志，write 返回
立即 kill -9 进程
检查日志文件
预期结果：该日志已经出现在文件中（依赖 OS 页缓存，符合“提交到操作系统”定义即可，不保证掉电不丢失）
## 4.20 可靠性测试-02
用例名称：连续轮转 1000 次，文件处理正确无泄漏
前置条件：容量 1KB，最大文件数 5
测试步骤：循环写入触发轮转，共 1000 次
预期结果：目录下始终最多 5 个日志文件，文件名和内容正确；无文件描述符泄漏；日志模块内存稳定
# 五、遗留任务
## 5.1 学习任务：
    学习论文：Flogger: A Fast Logger for Database Systems
## 5.2 功能任务
    - 支持运行时动态调整日志级别，通过pragma语句
    - 多文件轮转功能没做，因为write也需要加锁，影响性能，后面再考虑怎么做