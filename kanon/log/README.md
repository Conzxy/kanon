# Log Module
`Logger`类管理缓冲区和信息格式，如何输出（比如输出方式和输出设备）以及刷新缓冲区则由用户提供的回调控制。
```cpp
using OutputCallback = std::function<void(char const*, size_t)>;
using FlushCallback  = std::function<void()>;
```

这里预设了三种输出方式：
* `default`：默认设置是输出到stdout，刷新stdout。
* `LogFile`类，需要用户提供
  * 当前文件名，但是没有路径信息。
  * 日志滚动阈值，达到该值则生成新的日志文件并写入。
  * （可选）日志文件的存放位置，以"/"作为结尾， e.g. "xxx/"（default: 当前目录）
  * （可选）日志存放阈值，达到该阈值会自动清除除最新日志以外的其他日志文件（default：UINT_MAX，即不删除）
  * （可选）滚动周期，满一个周期，生成新的日志文件（由于精度是秒，如果写太快，会失效）（default：1 day）
  * （可选）刷缓冲周期，满一个周期，将缓冲区的内容刷到文件（default：3s）
* `AsyncLog`类，类似LogFile，但是是异步写入日志，使用了多缓冲技术（multiplex buffering），性能有一定的提升。参数同LogFile。

可以通过`SetupAsyncLog()`和`SetupLogFile()`启用对应的日志方式