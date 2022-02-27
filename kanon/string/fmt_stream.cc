#include "fmt_stream.h"
#include "kanon/string/stream_common.h"

namespace kanon {

template class FmtStream<kSmallStreamSize>;
template class FmtStream<kLargeStreamSize>;

} // namespace kanon