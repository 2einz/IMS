#pragma once

#include <cstdint>
#include <cstdio>
#include <memory>
#include <mutex>
#include <ostream>
#include <sstream>
#include <string>
#include <sys/types.h>
#include <thread>
#include <vector>

#include "fmt/base.h"
#include "fmt/chrono.h"

#include "../util.h"

namespace reinz {

class Logger;
class LogAppender;
class LogFormatter;
class LoggerManager;
class LogQueue;
class LogEvent;

class LogLevel {
public:
  enum Level {
    UNKNOWN = 0,
    DEBUG = 1,
    INFO = 2,
    WARN = 3,
    ERROR = 4,
    FATAL = 5
  };
};

/**
 * @brief 将string类型转化为日志级别
 */
static LogLevel::Level fromString(const std::string &str);

class LogEvent {
public:
  LogEvent(std::shared_ptr<Logger> logger, LogLevel::Level level,
           const char *file, uint32_t line, uint32_t elapse,
           std::thread::id thread_id, /*uint32_t fiber_id,*/ std::time_t time,
           const std::string &thread_name);

  const char *getFile() const { return file_; }
  int32_t getLine() const { return line_; }
  uint32_t getElapse() const { return elapse_; }
  std::thread::id getThreadId() const { return thread_id_; }
  // uint32_t getFiberId() const { return fiber_id_; }
  std::time_t getTime() const { return time_; }
  std::string getContent() const { return ss_.str(); }
  std::stringstream &getSS() { return ss_; }
  std::shared_ptr<Logger> getLogger() const { return logger_; }
  const std::string &getThreadName() const { return thread_name_; }
  LogLevel::Level getLevel() const { return level_; }

private:
  const char *file_ = nullptr; // 文件名
  uint32_t line_ = 0;          // 行号
  uint32_t elapse_ = 0;        // 程序启动到现在的毫秒数
  std::thread::id thread_id_;  // 线程id
  uint32_t fiber_id = 0;       // 协程id
  std::time_t time_ = 0;       // 时间戳
  std::string thread_name_;
  std::stringstream ss_;
  std::shared_ptr<Logger> logger_;
  LogLevel::Level level_;
};

class LogAppender {
  friend class Logger;

public:
  virtual ~LogAppender() = default;

  /**
   * @brief 写入日志
   * @param logger 日志器
   * @param level 日志级别
   * @param event 日志事件
   */
  virtual void log(std::shared_ptr<Logger> logger, LogLevel::Level level,
                   std::shared_ptr<LogEvent> event) = 0;

  /**
   * @brief 将日志输出目标的配置转成YAML String
   */
  virtual std::string toYamlString() = 0;

  LogLevel::Level getLevel() const { return level_; }
  void setLevel(const LogLevel::Level level) { level_ = level; }

  /**
   * @brief 获得日志格式器
   */
  std::shared_ptr<LogFormatter> getFormatter();

  /**
   * 更改日志格式器
   * @param formatter 日志格式器
   */
  void setFormatter(std::shared_ptr<LogFormatter> formatter);

protected:
  LogLevel::Level level_ = LogLevel::DEBUG; // 日志级别
  bool has_formatter_ = false;              // 是否有日志格式器
  std::shared_ptr<LogFormatter> formatter_; // 日志格式器
  std::unique_ptr<SpinLock> lock_;
};

// 日志队列
class LogQueue {
public:
  void push();
};

// 日志格式化
class LogFormatter {
public:
  /**
   * @brief 构造函数
   * @param pattern 格式模板
   * @details
   *  %m 消息
   *  %p 日志级别
   *  %r 累计毫秒数
   *  %c 日志名称
   *  %t 线程id
   *  %n 换行
   *  %d 时间
   *  %f 文件名
   *  %l 行号
   *  %T 制表符
   *  %F 协程id
   *  %N 线程名称
   *
   *  默认格式 "%d{%Y-%m-%d %H:%M:%S}%T%t%T%N%T%F%T[%p]%T[%c]%T%f:%l%T%m%n"
   */
  LogFormatter(const std::string &pattern);

public:
  // 格式项基类
  class FormatItem {
  public:
    virtual ~FormatItem() = default;
    virtual void format(std::ostream &os, const std::shared_ptr<Logger> &logger,
                        LogLevel::Level level,
                        const std::shared_ptr<LogEvent> &event) = 0;
  };

private:
  std::string pattern_;
  std::vector<std::shared_ptr<FormatItem>> items_;
  bool error_ = false; // 是否存在错误
};

class NameFormatItem final : public LogFormatter::FormatItem {
public:
  explicit NameFormatItem(const std::string &str = "") {};
  void format(std::ostream &os, const std::shared_ptr<Logger> &logger,
              LogLevel::Level level,
              const std::shared_ptr<LogEvent> &event) override {
                // Todo:
              }
};

} // namespace reinz