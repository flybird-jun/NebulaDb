#include <iomanip>
#include <chrono>
#include <format>
#include <iostream>
#include <filesystem>

#include <sys/types.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "logger.h"
namespace nebula {

Logger &Logger::get_logger() {
    static Logger *instance = new Logger();
    return *instance;
}

Status Logger::init(const std::string &path, const std::string &file_name, Level level, [[maybe_unused]] int file_count, [[maybe_unused]] int capacity)
{
    // 如果已经初始化过，先清理旧状态(便于测试和重新配置)
    if (inited_ ) {
        return NEBULA_OK;
    }
    std::filesystem::path dir_path = path;
    if (!std::filesystem::is_directory(dir_path)) {
        if (std::filesystem::exists(dir_path)) {
            std::cout << "exist " << dir_path << " file in init Logger" << std::endl;
            return NEBULA_INVALID_ARGS;
        }
        // 创建目录
        if (!std::filesystem::create_directories(path)) {
            std::cout << "create " << dir_path << " file failed in init Logger" << std::endl;
            return NEBULA_INVALID_ARGS;
        }
    }
    std::string file_path = path + '/' + file_name + ".log";
    fd_ = ::open(file_path.c_str(), O_WRONLY | O_CREAT | O_APPEND, 0644);
    if (fd_ < 0) {
        std::cout << "open " << dir_path << " failed in init Logger" << std::endl;
        return NEBULA_INVALID_ARGS;
    }
    path_ = path;
    file_name_ = file_name;
    effect_level_ = level;
    pid_ = ::getpid();
    inited_ = true;
    return NEBULA_OK;
}

// 日志格式: time [Level][MODULE] pid tid | [Status] message\n
void Logger::write(Level level, Status ret, const std::string &module, std::string_view fmt, std::format_args &&args) {
    if (!inited_ || fd_ < 0 || level < effect_level_) {
        return;
    }
    std::string_view level_string[] = {"DEBUG", "INFO", "ERROR"};
    auto now = std::chrono::system_clock::now();
    auto zt = std::chrono::zoned_time{std::chrono::current_zone(), now};

    auto log = std::format("{} [{}] [{}] {} {} | [ERROR-{}] {}\n",
        std::format("{:%Y-%m-%d %H:%M:%S}", zt),
        level_string[level],
        module,
        pid_,
        static_cast<uint64_t>(pthread_self()),
        static_cast<uint32_t>(ret),
        std::vformat(fmt, args));

    ::write(fd_, log.data(), log.length());
}

Logger::~Logger() {
    ::close(fd_);
    inited_ = false;
}

} // namespace nebula
