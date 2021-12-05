#include "lexical_stream.h"

namespace kanon {

template class LexicalStream<kSmallStreamSize>;
template class LexicalStream<kLargeStreamSize>;
template class LexicalStream<CAST_FIXEDBUFFER_SIZE>;

} // namespace kanon
