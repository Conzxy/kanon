#ifndef KANON_LOG_ASYNCLOG_TRIGGER_H
#define KANON_LOG_ASYNCLOG_TRIGGER_H

#include "kanon/util/noncopyable.h"
#include "kanon/log/Logger.h"
#include "kanon/log/AsyncLog.h"

namespace kanon {

/**
 * @brief construct this to trigger asynchronous logging
 * @warning 
 *  -- This is must be not a temporary object
 *  -- construct before any logic, that is the first statement in main()
 */
class AsyncLogTrigger : noncopyable {
public:
  static AsyncLogTrigger& instance(StringView basename,
      size_t rollSize,
      StringView prefix = "",
      size_t flushInterval = 3,
      size_t rollLine = 1024) {
    static AsyncLogTrigger ret{ basename, rollSize, prefix, flushInterval, rollLine };

    return ret;
  }
private:
  AsyncLogTrigger(StringView basename,
                  size_t rollSize,
                  StringView prefix = "",
                  size_t flushInterval = 3,
                  size_t rollLine = 1024)
    : log_{ basename, rollSize, prefix, flushInterval, rollLine }
  {
    Logger::setFlushCallback([this](){
      this->log_.flush();
    });

    Logger::setOutputCallback([this](char const* data, size_t len) {
      this->log_.append(data, len);
    });

    log_.start();
  }

  AsyncLog log_;
};

} // namespace kanon

#endif // KANON_LOG_ASYNCLOG_TRIGGER_H
