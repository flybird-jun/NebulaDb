#pragma once

#include <mutex>
#include <string>
#include <format>
#include <sys/types.h>
#include "db_define.h"

namespace nebula {
// 单例日志模块，提供同步、低延迟的日志打印能力
class Logger {
public:
    enum Level {
        DEBUG = 0,
        INFO = 1,
        ERROR = 2,
    };

    // 获取全局唯一的Logger实例
    static Logger &get_logger();

    // 禁止拷贝与移动
    Logger(const Logger &) = delete;
    Logger &operator=(const Logger &) = delete;
    Logger(Logger &&) = delete;
    Logger &operator=(Logger &&) = delete;

    // 初始化日志模块(指定文件名前缀)
    Status init(const std::string &path, const std::string &file_name, Level level, int file_count, int capacity);

    // 写入一条日志
    // level: 日志级别
    // module: 模块标签，写入 [MODULE] 字段
    // format: printf 风格格式化字符串
    void write(Level level, Status ret, const std::string &module, std::string_view fmt, std::format_args&& args);

    // 析构前需要flush所有未写入数据
    ~Logger();

private:
    Logger() = default;

    std::string path_;          // 日志目录
    std::string file_name_;     // 日志文件名前缀
    [[maybe_unused]] int file_count_ = 0;        // 轮转日志文件最大数量
    [[maybe_unused]] int cur_file_hwm_ = 0;      // 当前已使用过的最大文件编号
    [[maybe_unused]] int file_capacity_ = 0;     // 单个文件最大占用空间，单位为KB
    pid_t pid_ = 0;             // 进程pid
    Level effect_level_ = INFO; // 有效日志级别

    [[maybe_unused]] std::mutex mutex_; //
    int fd_ = -1;               // 当前活动日志文件fd
    bool inited_ = false;       // 是否已经初始化
};

#define LOG_DEBUG(format, ...) Logger::get_logger().write(Logger::DEBUG, NEBULA_OK, MODULE_NAME, format, std::make_format_args(__VA_ARGS__))
#define LOG_INFO(format, ...) Logger::get_logger().write(Logger::INFO, NEBULA_OK, MODULE_NAME, format, std::make_format_args(__VA_ARGS__))
#define LOG_ERROR(ret, format, ...) Logger::get_logger().write(Logger::ERROR, ret, MODULE_NAME, format, std::make_format_args(__VA_ARGS__))

}
