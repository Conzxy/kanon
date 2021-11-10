#ifndef KANON_LOGFILE_TRIGGER_H
#define KANON_LOGFILE_TRIGGER_H

#include "kanon/util/macro.h"
#include "kanon/util/noncopyable.h"
#include "kanon/log/LogFile.h"
#include "kanon/log/Logger.h"

namespace kanon {

template<bool ThreadSafe=false>
class LogFileTrigger : noncopyable {
public:
  LogFileTrigger(StringView basename,
    size_t rollSize,
    StringView prefix)
    : logFile_{ basename, rollSize, prefix }
  { 
    Logger::setOutputCallback([this](char const* data, size_t num) {
        logFile_.append(data, num);
    });

    Logger::setFlushCallback([this]() {
        logFile_.flush();
    });
  }

private:
  LogFile<ThreadSafe> logFile_;
};

} // namespace kanon

#endif // KANON_LOGFILE_TRIGGER_H
