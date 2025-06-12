
#pragma once

#include <fmt/chrono.h>
#include <fmt/format.h>
#include <fmt/ostream.h>
#include <sys/types.h>

#include <cstdint>
#include <cstdio>
#include <functional>
#include <iostream>
#include <list>
#include <memory>
// #include <mutex>
#include <ostream>
#include <sstream>
#include <string>
#include <thread>
#include <vector>

#include "../util.h"
#include "fmt/base.h"

namespace Logging {

using MutexType = reinz::SpinLock;
using MutexGuard = reinz::SpinlockGuard;

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

  /**
   * @brief 将string类型转化为日志级别
   */
  static LogLevel::Level fromString(const std::string& str);
  /**
   * @brief 将日志级别转换为文本输出
   */
  static const char* toString(LogLevel::Level level);

  // Todo: 为fmt库提供格式化支持
};

class LogEvent {
 public:
  LogEvent(std::shared_ptr<Logger> logger, LogLevel::Level level,
           const char* file, uint32_t line, uint32_t elapse,
           std::thread::id thread_id, /*uint32_t fiber_id,*/ std::time_t time,
           const std::string& thread_name);

  const char* getFile() const { return file_; }

  int32_t getLine() const { return line_; }

  uint32_t getElapse() const { return elapse_; }

  std::thread::id getThreadId() const { return thread_id_; }

  // uint32_t getFiberId() const { return fiber_id_; }
  std::chrono::system_clock::time_point getTime() const { return time_; }

  std::string getContent() const { return ss_.str(); }

  std::stringstream& getSS() { return ss_; }

  std::shared_ptr<Logger> getLogger() const { return logger_; }

  const std::string& getThreadName() const { return thread_name_; }

  LogLevel::Level getLevel() const { return level_; }

 private:
  const char* file_ = nullptr;                  // 文件名
  uint32_t line_ = 0;                           // 行号
  uint32_t elapse_ = 0;                         // 程序启动到现在的毫秒数
  std::thread::id thread_id_;                   // 线程id
  uint32_t fiber_id = 0;                        // 协程id
  std::chrono::system_clock::time_point time_;  // 时间戳
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
  LogLevel::Level level_ = LogLevel::DEBUG;  // 日志级别
  bool has_formatter_ = false;               // 是否有日志格式器
  std::shared_ptr<LogFormatter> formatter_;  // 日志格式器
  MutexType lock_;
};

class Logger : public std::enable_shared_from_this<Logger> {
  friend class LoggerManager;

 public:
  Logger(const std::string& name = "root");

  void log(LogLevel::Level level, std::shared_ptr<LogEvent> event);
  void debug(std::shared_ptr<LogEvent> event);
  void info(std::shared_ptr<LogEvent> event);
  void warn(std::shared_ptr<LogEvent> event);
  void fatal(std::shared_ptr<LogEvent> event);

  void addAppender(std::shared_ptr<LogAppender> appender);
  void delAppender(std::shared_ptr<LogAppender> appender);
  void clearAppender();

  LogLevel::Level getLevel() const { return level_; }

  void setLevel(LogLevel::Level level) { level_ = level; }

  const std::string& getName() const { return name_; }

  /**
   * @brief 设置日志格式器
   */
  void setFormatter(std::shared_ptr<LogFormatter> formatter);
  /*
   * @brief 设置日志格式
   */
  void setFormatter(const std::string& fmt);
  std::shared_ptr<LogFormatter> getFormatter();
  std::string toYamlString();

 private:
  std::string name_;
  LogLevel::Level level_;
  MutexType lock_;
  std::list<std::shared_ptr<LogAppender>> appenders_;
  std::shared_ptr<LogFormatter> formatter;
  std::shared_ptr<Logger> root_;
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
  LogFormatter(const std::string& pattern);

  void init();

  bool isError() const { return error_; }

  const std::string& getPattern() const { return pattern_; }

  std::string format(std::shared_ptr<Logger> logger, LogLevel::Level level,
                     std::shared_ptr<LogEvent> event);

 public:
  // 格式项基类
  class FormatItem {
   public:
    virtual ~FormatItem() = default;
    virtual void format(std::ostream& os, const std::shared_ptr<Logger>& logger,
                        LogLevel::Level level,
                        const std::shared_ptr<LogEvent>& event);
    virtual void format(fmt::memory_buffer& buffer,
                        const std::shared_ptr<Logger> logger,
                        LogLevel::Level level,
                        std::shared_ptr<LogEvent> event) = 0;
  };

 private:
  std::string pattern_;
  std::vector<std::shared_ptr<FormatItem>> items_;
  bool error_ = false;  // 是否存在错误

  // 定义工厂函数类型
  using FormatFactory =
      std::function<std::shared_ptr<FormatItem>(const std::string&)>;

  // 模板函数生成工厂函数
  template <typename T>
  static FormatFactory make_factory() {
    return [](const std::string& fmt) { return std::make_shared<T>(fmt); };
  };
};  // class LogFormatter

class NameFormatItem final : public LogFormatter::FormatItem {
 public:
  explicit NameFormatItem(const std::string& str = "") {};

  void format(std::ostream& os, const std::shared_ptr<Logger>& logger,
              LogLevel::Level level,
              const std::shared_ptr<LogEvent>& event) override {
    fmt::print(os, "{}", event->getLogger()->getName());
  }
};

class DateTimeFormatItem final : public LogFormatter::FormatItem {
 public:
  explicit DateTimeFormatItem(const std::string& format = "%Y-%m-%d %H:%M:%S")
      : format_(std::move(format)) {
    if (format_.empty()) {
      format_ = "%Y-%m-%d %H:%M:%S";
    }
  }

  void format(std::ostream& os, const std::shared_ptr<Logger>& logger,
              LogLevel::Level level,
              const std::shared_ptr<LogEvent>& event) override {
    fmt::print(os, format_, event->getTime());
  }

  void format(fmt::memory_buffer& buffer, const std::shared_ptr<Logger> logger,
              LogLevel::Level level, std::shared_ptr<LogEvent> event) override {
    fmt::format_to(std::back_inserter(buffer), format_, event->getTime());
  }

 private:
  std::string format_;
};

class FilenameFormatItem final : public LogFormatter::FormatItem {
 public:
  explicit FilenameFormatItem(const std::string& str = "") {}

  void format(std::ostream& os, const std::shared_ptr<Logger>& logger,
              LogLevel::Level level,
              const std::shared_ptr<LogEvent>& event) override {
    fmt::print(os, "{}", event->getFile());
  }
  void format(fmt::memory_buffer& buffer, const std::shared_ptr<Logger> logger,
              LogLevel::Level level, std::shared_ptr<LogEvent> event) override {
    fmt::format_to(std::back_inserter(buffer), "{}", event->getFile());
  }
};

class LineFormatItem final : public LogFormatter::FormatItem {
 public:
  explicit LineFormatItem(const std::string& str = "") {}

  void format(std::ostream& os, const std::shared_ptr<Logger>& logger,
              LogLevel::Level level,
              const std::shared_ptr<LogEvent>& event) override {
    fmt::print(os, "{}", event->getLine());
  }
  void format(fmt::memory_buffer& buffer, const std::shared_ptr<Logger> logger,
              LogLevel::Level level, std::shared_ptr<LogEvent> event) override {
    fmt::format_to(std::back_inserter(buffer), "{}", event->getLine());
  }
};

class NewLineFormatItem final : public LogFormatter::FormatItem {
 public:
  explicit NewLineFormatItem(const std::string& str = "") {}

  void format(std::ostream& os, const std::shared_ptr<Logger>& logger,
              LogLevel::Level level,
              const std::shared_ptr<LogEvent>& event) override {
    fmt::print(os, "\n");
  }
  void format(fmt::memory_buffer& buffer, const std::shared_ptr<Logger> logger,
              LogLevel::Level level, std::shared_ptr<LogEvent> event) override {
    fmt::format_to(std::back_inserter(buffer), "\n");
  }
};

class MessageFormatItem final : public LogFormatter::FormatItem {
 public:
  explicit MessageFormatItem(const std::string& str = "") {};

  void format(std::ostream& os, const std::shared_ptr<Logger>& logger,
              LogLevel::Level level,
              const std::shared_ptr<LogEvent>& event) override {
    fmt::print(os, "{}", event->getContent());
  }
  void format(fmt::memory_buffer& buffer, const std::shared_ptr<Logger> logger,
              LogLevel::Level level, std::shared_ptr<LogEvent> event) override {
    fmt::format_to(std::back_inserter(buffer), "{}", event->getContent());
  }
};

class ThreadIdFormatItem final : public LogFormatter::FormatItem {
 public:
  explicit ThreadIdFormatItem(const std::string& str = "") {};

  void format(std::ostream& os, const std::shared_ptr<Logger>& logger,
              LogLevel::Level level,
              const std::shared_ptr<LogEvent>& event) override {
    fmt ::print(os, "{}", event->getThreadId());
  }
  void format(fmt::memory_buffer& buffer, const std::shared_ptr<Logger> logger,
              LogLevel::Level level, std::shared_ptr<LogEvent> event) override {
    fmt::format_to(std::back_inserter(buffer), "{}", event->getThreadId());
  }
};

class LevelFormatItem final : public LogFormatter::FormatItem {
 public:
  explicit LevelFormatItem(const std::string& str = "") {};

  void format(std::ostream& os, const std::shared_ptr<Logger>& logger,
              LogLevel::Level level,
              const std::shared_ptr<LogEvent>& event) override {
    fmt::print(os, "{}", LogLevel::toString(level));
  }
  void format(fmt::memory_buffer& buffer, const std::shared_ptr<Logger> logger,
              LogLevel::Level level, std::shared_ptr<LogEvent> event) override {
    fmt::format_to(std::back_inserter(buffer), "{}", LogLevel::toString(level));
  }
};

class ElapseFormatItem final : public LogFormatter::FormatItem {
 public:
  explicit ElapseFormatItem(const std::string& str = "") {};

  void format(std::ostream& os, const std::shared_ptr<Logger>& logger,
              LogLevel::Level level,
              const std::shared_ptr<LogEvent>& event) override {
    fmt::print(os, "{}", event->getElapse());
  }
  void format(fmt::memory_buffer& buffer, const std::shared_ptr<Logger> logger,
              LogLevel::Level level, std::shared_ptr<LogEvent> event) override {
    fmt::format_to(std::back_inserter(buffer), "{}", event->getElapse());
  }
};

class TabFormatItem final : public LogFormatter::FormatItem {
 public:
  explicit TabFormatItem() {}

  void format(std::ostream& os, const std::shared_ptr<Logger>& logger,
              LogLevel::Level level,
              const std::shared_ptr<LogEvent>& event) override {
    fmt::print(os, "\t");
  }
  void format(fmt::memory_buffer& buffer, const std::shared_ptr<Logger> logger,
              LogLevel::Level level, std::shared_ptr<LogEvent> event) override {
    fmt::format_to(std::back_inserter(buffer), "\t");
  }
};

class ThreadNameFormatItem final : public LogFormatter::FormatItem {
 public:
  explicit ThreadNameFormatItem(const std::string& str = "") {}

  void format(std::ostream& os, const std::shared_ptr<Logger>& logger,
              LogLevel::Level level,
              const std::shared_ptr<LogEvent>& event) override {
    fmt::print(os, "{}", event->getThreadName());
  }
  void format(fmt::memory_buffer& buffer, const std::shared_ptr<Logger> logger,
              LogLevel::Level level, std::shared_ptr<LogEvent> event) override {
    fmt::format_to(std::back_inserter(buffer), "{}", event->getThreadName());
  }
};

// class FiberIdFormatItem final : public LogFormatter::FormatItem {
//  public:
//   explicit FiberIdFormatItem(const std::string &str = "") {};
//   void format(std::ostream &os, const std::shared_ptr<Logger> &logger,
//               LogLevel::Level level,
//               const std::shared_ptr<LogEvent> &event) override {
//     fmt::print(os, "{}", event->getFiberId());
//   }
// void format(fmt::memory_buffer& buffer, const std::shared_ptr<Logger> logger,
//             LogLevel::Level level, std::shared_ptr<LogEvent> event) override
//             {
//   fmt::format_to(std::back_inserter(buffer), "{}", event->getFiberId());
// }
// };

class StringFormatItem final : public LogFormatter::FormatItem {
 public:
  explicit StringFormatItem(const std::string& str) : string_(str) {}

  void format(std::ostream& os, const std::shared_ptr<Logger>& logger,
              LogLevel::Level level,
              const std::shared_ptr<LogEvent>& event) override {
    fmt::print(os, "{}", string_);
  }
  void format(fmt::memory_buffer& buffer, const std::shared_ptr<Logger> logger,
              LogLevel::Level level, std::shared_ptr<LogEvent> event) override {
    fmt::format_to(std::back_inserter(buffer), "{}", string_);
  }

 private:
  std::string string_;
};

/**
 * @brief 将单个参数转化为字符串
 */
template <typename T>
std::string toString(T&& arg) {
  std::stringstream ss;
  ss << std::forward<T>(arg);
  return ss.str();
}

};  // namespace Logging