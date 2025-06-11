#include "log.h"
// #include "../util.h"

#include <algorithm>
#include <memory>
// #include <boost/bimap.hpp>

#include "log.hpp"
namespace reinz {

static LogLevel::Level fromString(const std::string &str) {
#define XX(level, str)                                                         \
  if (s == #str)                                                               \
    return LogLevel::level;
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

std::shared_ptr<LogFormatter> LogAppender::getFormatter() {
  std::lock_guard<SpinLock> guard(*lock_);
  return formatter_;
}

void LogAppender::setFormatter(std::shared_ptr<LogFormatter> formatter) {
  std::lock_guard<SpinLock> guard(*lock_);
  formatter_ = formatter;
  has_formatter_ = formatter_ != nullptr ? true : false;
}

} // namespace reinz