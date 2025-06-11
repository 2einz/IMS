
#pragma once

#include <iostream>
#include <sstream>
#include <string>

namespace rein {

  /**
  * @brief 将单个参数转化为字符串
  */
  template<typename T>
  std::string toString(T&& arg) {
    std::stringstream ss;
    ss << std::forward<T>(arg);
    return ss.str();
  }

};