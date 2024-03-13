#pragma once

#include <cstring>
#include <iostream>

inline auto ASSERT(bool cond, const std::string &msg) noexcept {
  if (!cond) [[unlikely]] {
    std::cerr << "ASSERT : " << msg << std::endl;

    exit(EXIT_FAILURE);
  }
}

inline auto FATAL(const std::string &msg) noexcept {
  std::cerr << "FATAL : " << msg << std::endl;

  exit(EXIT_FAILURE);
}