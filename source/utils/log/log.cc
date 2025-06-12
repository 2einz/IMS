#include "log.hpp"
// #include "../util.h"

#include <algorithm>
#include <cctype>
#include <cstddef>
#include <functional>
#include <iterator>
#include <map>
#include <memory>
#include <sstream>
#include <tuple>
#include <vector>
// #include <boost/bimap.hpp>

namespace Logging {

static LogLevel::Level fromString(const std::string &str) {
#define XX(level, str) \
  if (s == #str) return LogLevel::level;
  std::string s = str;
  std::transform(str.begin(), str.end(), str.begin(), ::tolower);
  XX(DEBUG, debug)
  XX(INFO, info)
  XX(WARN, warn)
  XX(ERROR, error)
  XX(FATAL, fatal)

  return LogLevel::UNKNOWN;
#undef XX
}

static const char *toString(LogLevel::Level level) {
  switch (level) {
#define XX(name)       \
  case LogLevel::name: \
    return #name;

    XX(DEBUG);
    XX(INFO);
    XX(WARN);
    XX(ERROR);
    XX(FATAL);
#undef XX
    default:
      return "UNKNOWN";
  }
  return "UNKNOWN";
}

std::shared_ptr<LogFormatter> LogAppender::getFormatter() {
  MutexGuard guard(lock_);
  return formatter_;
}

void LogAppender::setFormatter(std::shared_ptr<LogFormatter> formatter) {
  MutexGuard guard(lock_);
  formatter_ = formatter;
  has_formatter_ = formatter_ != nullptr ? true : false;
}

Logger::Logger(const std::string &name) : name_(name), level_(LogLevel::DEBUG) {
  // formatter_.reset(new LogFormatter(
  //     "%d{%Y-%m-%d %H:%M:%S}%T%t%T%N%T%T[%p]%T[%c]%T%f:%l%T%m%n"));
  formatter_ = std::make_shared<LogFormatter>(
      "%d{%Y-%m-%d %H:%M:%S}%T%t%T[%p]%T[%c]%T%f:%l%T%m%n");
}

void Logger::log(LogLevel::Level level, std::shared_ptr<LogEvent> event) {
  if (level < level_) return;
  {
    MutexGuard lock(lock_);

    if (!appenders_.empty()) {
      auto self = shared_from_this();  // 仅在需要的时候才获取共享指针
      for (auto &appender : appenders_) {
        appender->log(self, level, event);
      }
      return;
    }
  }
  // 没有appenders且有root_logger_ 则转发到root_logger
  if (root_logger_) {
    root_logger_->log(level, event);
  }
}

void Logger::setFormatter(std::shared_ptr<LogFormatter> formatter) {
  {
    MutexGuard lock(lock_);
    formatter_ = formatter;
  }
  for (auto &i : appenders_) {
    MutexGuard appender_lock(i->lock_);
    if (!i->has_formatter_) {
      i->formatter_ = formatter_;
    }
  }
}

LogFormatter::LogFormatter(const std::string &patter)
    : pattern_(std::move(patter)) {
  init();
}

void LogFormatter::init() {
  // str, format, type
  // "%d{%%Y-%%m-%%d} [%p] %f:%l%m%n"
  //
  std::vector<std::tuple<std::string, std::string, int>> parsed_patterns;
  std::string literal_buffer;

  // factory mapping
  static const std::map<std::string, FormatFactory> format_factories = {
      //{"F", make_factory<FiberIdFormatItem>()},     // F:协程id
      {"c", make_factory<NameFormatItem>()},        // c:日志名称
      {"d", make_factory<DateTimeFormatItem>()},    // d:时间
      {"f", make_factory<FilenameFormatItem>()},    // f:文件名
      {"l", make_factory<LineFormatItem>()},        // l:行号
      {"m", make_factory<MessageFormatItem>()},     // m:消息
      {"n", make_factory<NewLineFormatItem>()},     // n:换行
      {"N", make_factory<ThreadNameFormatItem>()},  // N:线程名称
      {"p", make_factory<LevelFormatItem>()},       // p:日志级别
      {"r", make_factory<ElapseFormatItem>()},      // r:累计毫秒数
      {"T", make_factory<TabFormatItem>()},         // T:Tab
      {"t", make_factory<ThreadIdFormatItem>()},    // t:线程id
  };

  auto commitLiteral = [&] {
    if (!literal_buffer.empty()) {
      parsed_patterns.emplace_back(literal_buffer, "", 0);
      literal_buffer.clear();
    }
  };

  for (size_t i = 0; i < pattern_.size();) {
    // 普通字符累计逻辑
    if (pattern_[i] != '%') {
      literal_buffer += pattern_[i++];
      continue;
    }

    // 遇到%符号出发格式解析
    commitLiteral();

    // 转义处理 %%->%
    if ((i + 1) < pattern_.size() && pattern_[i + 1] == '%') {
      literal_buffer += '%';
      i += 2;
      continue;
    }

    // 核心解析
    size_t n = i + 1;
    int fmt_status = 0;  // 0-解析标识符，1-解析格式参数
    size_t fmt_begin = i + 1;

    std::string specifier, fmt_param;

    while (n < pattern_.size()) {
      char c = pattern_[n];
      if (fmt_status == 0) {
        if (c == '{') {
          specifier = pattern_.substr(i + 1, n - i - 1);
          fmt_status = 1;
          fmt_begin = n + 1;  // 记录参数开始位置
          n++;
        } else if (!std::isalpha(c)) {
          // 非字母字符结束 specifier解析
          specifier = pattern_.substr(i + 1, n - i - 1);
          break;
        } else {
          n++;
        }
      } else if (fmt_status == 1) {
        if (c == '}') {
          fmt_param = pattern_.substr(fmt_begin, n - fmt_begin);
          fmt_status = 0;
          n++;  // 跳过闭合括号
          break;
        } else {
          n++;
        }
      }
    }

    // 处理未闭合的 { 或 末尾的 specifier
    if (specifier.empty() && n > i + 1) {
      // 捕获纯字母 (eg. %n)
      specifier = pattern_.substr(i + 1, n - i - 1);
    }

    if (!specifier.empty()) {
      parsed_patterns.emplace_back(specifier, fmt_param, 1);
    } else {
      error_ = true;
      parsed_patterns.emplace_back("<<pattern_error>>", "", 0);
    }

    i = n;  // 更新外层循环索引

    commitLiteral();
  }

  // 构建最终格式项列表
  for (const auto &item : parsed_patterns) {
    const auto &str = std::get<0>(item);
    const auto &fmt = std::get<1>(item);
    const int type = std::get<2>(item);

    if (type == 0) {
      items_.push_back(std::make_shared<StringFormatItem>(str));
    } else {
      auto it = format_factories.find(str);
      if (it != format_factories.end()) {
        items_.push_back(it->second(fmt));
      } else {
        error_ = true;
        items_.push_back(std::make_shared<StringFormatItem>("<<error_format %" +
                                                            str + ">>"));
      }
    }
  }
}

std::string LogFormatter::format(std::shared_ptr<Logger> logger,
                                 LogLevel::Level level,
                                 std::shared_ptr<LogEvent> event) {
  fmt::memory_buffer buffer;

  for (const auto &item : items_) {
    item->format(buffer, logger, level, event);
  }
  return fmt::to_string(buffer);
}

}  // namespace Logging