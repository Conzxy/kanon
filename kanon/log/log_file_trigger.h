#ifndef KANON_LOGFILE_TRIGGER_H
#define KANON_LOGFILE_TRIGGER_H

#include "kanon/util/macro.h"
#include "kanon/util/noncopyable.h"
#include "kanon/log/log_file.h"
#include "kanon/log/logger.h"

namespace kanon {

template<bool ThreadSafe=false>
class LogFileTrigger : noncopyable {
public:
  LogFileTrigger(StringView basename,
    size_t rollSize)
    : LogFileTrigger(basename, rollSize, StringView{})
  { }

  LogFileTrigger(StringView basename,
    size_t rollSize,
    StringView prefix)
    : logFile_{ basename, rollSize, prefix }
  { 
    Logger::SetOutputCallback([this](char const* data, size_t num) {
        logFile_.Append(data, num);
    });

    Logger::SetFlushCallback([this]() {
        logFile_.flush();
    });
  }

private:
  LogFile<ThreadSafe> logFile_;
};

} // namespace kanon

#endif // KANON_LOGFILE_TRIGGER_H
