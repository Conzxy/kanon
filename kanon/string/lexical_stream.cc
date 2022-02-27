#include "lexical_stream.h"

namespace kanon {

template class LexicalStream<kSmallStreamSize>;
template class LexicalStream<kLargeStreamSize>;
template class LexicalStream<kCastStreamSize>;

} // namespace kanon
