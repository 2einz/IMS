#include "log.hpp"
// #include "../util.h"

#include <algorithm>
#include <functional>
#include <map>
#include <memory>
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
#define XX(name) \
  case LogLevel::name: return #name;

    XX(DEBUG);
    XX(INFO);
    XX(WARN);
    XX(ERROR);
    XX(FATAL);
#undef XX
  default: return "UNKNOWN";
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
  static const std::map<char, FormatFactory> format_factories = {
      //{'F', make_factory<FiberIdFormatItem>()},     // F:协程id
      {'c', make_factory<NameFormatItem>()},        // c:日志名称
      {'d', make_factory<DateTimeFormatItem>()},    // d:时间
      {'f', make_factory<FilenameFormatItem>()},    // f:文件名
      {'l', make_factory<LineFormatItem>()},        // l:行号
      {'m', make_factory<MessageFormatItem>()},     // m:消息
      {'n', make_factory<NewLineFormatItem>()},     // n:换行
      {'N', make_factory<ThreadNameFormatItem>()},  // N:线程名称
      {'p', make_factory<LevelFormatItem>()},       // p:日志级别
      {'r', make_factory<ElapseFormatItem>()},      // r:累计毫秒数
      {'T', make_factory<TabFormatItem>()},         // T:Tab
      {'t', make_factory<ThreadIdFormatItem>()},    // t:线程id
  };
}

}  // namespace Logging