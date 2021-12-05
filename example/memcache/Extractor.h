#ifndef KANON_EXAMPLE_MEMCACHE_EXTRACTOR_H
#define KANON_EXAMPLE_MEMCACHE_EXTRACTOR_H

#include "StringViewTokenizer.h"
#include "kanon/string/lexical_cast.h"
#include "kanon/util/optional.h"

namespace memcache {

template<char Separater>
class Extractor : kanon::noncopyable {
  typedef typename StringViewTokenizer<Separater>::iterator Iterator;
public:
  Extractor(Iterator const& beg, Iterator const& end)
    : beg_{ beg }
    , end_{ end }
  { }
  
  template<typename T>
  kanon::optional<T> extract() {
    if (beg_ == end_) {
      return kanon::make_null_optional<T>();
    }

    auto token = *(beg_++);
    
    return kanon::lexical_cast<T>(token);
  }

private:
  Iterator beg_;
  Iterator end_;
};

typedef Extractor<' '> SpaceExtractor;

} // namespace memcache

#endif // KANON_EXAMPLE_MEMCACHE_EXTRACTOR_H