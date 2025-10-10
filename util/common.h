#pragma once

// #include "spdlog/spdlog.h"

// using spdlog::info;
// using spdlog::error;
// using spdlog::warn;
// using spdlog::debug;
// using spdlog::trace;

#include <iostream>
#include <format>

#define info(fmt, ...) std::cout << std::format(fmt, ##__VA_ARGS__) << std::endl;
#define error(fmt, ...) std::cerr << std::format(fmt, ##__VA_ARGS__) << std::endl;
#define warn(fmt, ...) std::cerr << std::format(fmt, ##__VA_ARGS__) << std::endl;
#define debug(fmt, ...) std::cerr << std::format(fmt, ##__VA_ARGS__) << std::endl;
#define trace(fmt, ...) std::cerr << std::format(fmt, ##__VA_ARGS__) << std::endl;

#define DISALLOW_COPY_AND_ASSIGN(TypeName) \
  TypeName(const TypeName&) = delete;      \
  void operator=(const TypeName&) = delete
