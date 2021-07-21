//
//  Agora Real-time Engagement
//
//  Copyright (c) 2021 Agora.io. All rights reserved.
//

#pragma once

#include <atomic>
#include <chrono>
#include <ctime>
#include <functional>
#include <iomanip>
#include <mutex>
#include <shared_mutex>
#include <sstream>
#include <string>

namespace agora {
namespace rte {

// Logging level
//
enum class LogLevel : int {
  Verbose = 1,
  Info = 2,
  Warning = 3,
  Error = 4,
};

// Help class to print logs
//
class AgoraRteLogger {
 public:
  typedef std::function<void(const std::string& message)> LogListener;

  /**
   * @brief Set SDK logging level
   *
   * @param log_level The logging level
   */
  static void SetLevel(LogLevel log_level) {
    auto& log = GetLogger();
    log.log_level_ = log_level;
  }

  /**
   * @brief Enable or disable logs
   *
   * @param enabled Enable logs or not
   */
  static void EnableLogging(bool enabled) {
    auto& log = GetLogger();
    log.is_log_enabled_ = enabled;
  }

  /**
   * @brief Set the Listener object which SDK will redirect logs to
   *
   * @param listener Listener object
   */
  static void SetListener(LogListener listener) {
    auto& log = GetLogger();

#if RTE_MAC && (__cplusplus >= 201402L || _MSC_VER >= 1900)
    std::unique_lock<std::shared_timed_mutex> _(log.listener_lock_);
#endif
    log.log_listener_ = listener;
  }

  /**
   * @brief Write logs to listener when \p level >= SDK logging level
   *
   * @param level Log's logging level
   * @param message Log's message
   */
  static void Write(LogLevel level, const std::string& message) {
    auto& log = GetLogger();

#if RTE_MAC && (__cplusplus >= 201402L || _MSC_VER >= 1900)
    std::shared_lock<std::shared_timed_mutex> loggerLock(log.listener_lock_);
#endif

    if (log.log_listener_) {
      log.log_listener_(message);
    }
  }

  /**
   * @brief Write string message to listener
   *
   * @param message Log's message
   */
  static void Write(const std::string& message) {
    auto& log = GetLogger();

#if RTE_MAC && (__cplusplus >= 201402L || _MSC_VER >= 1900)
    std::shared_lock<std::shared_timed_mutex> loggerLock(log.listener_lock_);
#endif

    if (log.log_listener_) {
      log.log_listener_(message);
    }
  }

  /**
   * @brief Helper function to check logging level with SDK logging level
   *
   * @param level input logging level
   * @return true SDK should send the log to listener
   * @return false The log will be dropped
   */
  bool ShouldWriteLog(LogLevel level) const {
    return is_log_enabled_ && level >= log_level_;
  }

  static std::string GetNowTimeAsString() {
    auto now = std::chrono::system_clock::now();
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
      now.time_since_epoch()) %
              1000;
    auto timer = std::chrono::system_clock::to_time_t(now);

    std::tm bt = *std::localtime(&timer);

    std::ostringstream oss;

    oss << std::put_time(&bt, "%H:%M:%S");
    oss << '.' << std::setfill('0') << std::setw(3) << ms.count();

    return oss.str();
  }

  static inline std::string GetLabel(LogLevel level) {
    switch (level) {
      case LogLevel::Verbose:
        return "[ RTE VERB ]";
      case LogLevel::Info:
        return "[ RTE INFO ]";
      case LogLevel::Warning:
        return "[ RTE WARN ]";
      case LogLevel::Error:
        return "[ RTE ERROR]";
    }
    return "";
  }

  static AgoraRteLogger& GetLogger() {
    static AgoraRteLogger g_logger;
    return g_logger;
  }

  template <class T>
  AgoraRteLogger& operator <<(const T& message) {
    std::ostringstream temp_stream;

    temp_stream << message;

    Write(temp_stream.str());

    return *this;
  }

  //template <>
  //AgoraRteLogger& operator
  //<<<std::string>(
  //const std::string& message
  //) {
  //  Write(message);
  //  return *this;
  //}

 private:
  std::atomic<bool> is_log_enabled_ = {true};
  std::atomic<LogLevel> log_level_ = {LogLevel::Warning};

#if RTE_MAC && (__cplusplus >= 201402L || _MSC_VER >= 1900)
  std::shared_timed_mutex listener_lock_;
#endif

  AgoraRteLogger::LogListener log_listener_;

  AgoraRteLogger() = default;
  ~AgoraRteLogger() = default;

  AgoraRteLogger(const AgoraRteLogger&) = delete;
  AgoraRteLogger& operator =(const AgoraRteLogger&) = delete;
};

class LoggerFormat {
 public:
  LoggerFormat(agora::rte::LogLevel level, const std::string& function,
               const std::string& file, int line)
    : level_(level), function_(function), file_(file), line_(line) {}

  const std::string Format() {
    const auto str_level = AgoraRteLogger::GetLabel(level_);

    std::ostringstream format;
    format << str_level << "  " << AgoraRteLogger::GetNowTimeAsString() << "  "
           << file_.substr(file_.find_last_of('/') + 1)
           << ":" << line_ << "  "
           << (function_.empty() ? "" : (function_) + "  ");
    return format.str();
  }

  ~LoggerFormat() { AgoraRteLogger::GetLogger() << "\n"; }

 private:
  const agora::rte::LogLevel level_;
  const std::string& function_;
  const std::string& file_;
  const int line_;
};

#define RTE_LOG(sev)                                             \
  for (bool do_log = AgoraRteLogger::GetLogger().ShouldWriteLog( \
           agora::rte::LogLevel::sev);                           \
       do_log; do_log = false)                                   \
  AgoraRteLogger::GetLogger()                                    \
      << LoggerFormat(agora::rte::LogLevel::sev, __FUNCTION__, __FILE__, __LINE__).Format()

#define RTE_LOG_FUNC(sev, func)                                  \
  for (bool do_log = AgoraRteLogger::GetLogger().ShouldWriteLog( \
           agora::rte::LogLevel::sev);                           \
       do_log; do_log = false)                                   \
  AgoraRteLogger::GetLogger()                                    \
      << LoggerFormat(agora::rte::LogLevel::sev, #func, __FILE__, __LINE__).Format()

#define RTE_LOG_VERBOSE RTE_LOG(Verbose)
#define RTE_LOG_INFO RTE_LOG(Info)
#define RTE_LOG_WARNING RTE_LOG(Warning)
#define RTE_LOG_ERROR RTE_LOG(Error)

#define RTE_LOG_VERBOSE_FUNC(func) RTE_LOG_FUNC(Verbose, func)
#define RTE_LOG_INFO_FUNC(func) RTE_LOG_FUNC(Info, func)
#define RTE_LOG_WARNING_FUNC(func) RTE_LOG_FUNC(Warning, func)
#define RTE_LOG_ERROR_FUNC(func) RTE_LOG_FUNC(Error, func)

}  // namespace rte
}  // namespace agora
