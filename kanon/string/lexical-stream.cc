#include "lexical-stream.h"

namespace kanon {

template class LexicalStream<kSmallStreamSize>;
template class LexicalStream<kLargeStreamSize>;

} // namespace kanon
