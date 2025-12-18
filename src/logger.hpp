#pragma once

#include <quill/Backend.h>
#include <quill/Frontend.h>
#include <quill/LogFunctions.h>
#include <quill/Logger.h>
#include <quill/sinks/ConsoleSink.h>

inline quill::Logger* GetLogger() {
  static quill::Logger* logger{quill::Frontend::create_or_get_logger(
      "default_logger",
      quill::Frontend::create_or_get_sink<quill::ConsoleSink>("default_sink"))};
  return logger;
}

#define LOG_DEBUG(...) quill::debug(GetLogger(), __VA_ARGS__)
#define LOG_INFO(...) quill::info(GetLogger(), __VA_ARGS__)
#define LOG_WARNING(...) quill::warning(GetLogger(), __VA_ARGS__)
#define LOG_ERROR(...) quill::error(GetLogger(), __VA_ARGS__)
#define LOG_CRITICAL(...) quill::critical(GetLogger(), __VA_ARGS__)
