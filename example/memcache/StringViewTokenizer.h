#ifndef KANON_EXAMPLE_MEMCACHE_SPACE_SEPARATOR_H
#define KANON_EXAMPLE_MEMCACHE_SPACE_SEPARATOR_H

#include <boost/tokenizer.hpp>

#include "kanon/util/noncopyable.h"
#include "kanon/string/string_view.h"

namespace memcache {

template<char Separator>
class StringViewCharSeparator  {
public:
  void reset()
  {
    // There are no state variable need to reset
  }

  template<typename II, typename Token>
  bool operator()(II& first, II end, Token& tok) {
    // Find the first character which is not separator
    for (; first != end && *first == Separator; ++first);

    if (first == end) {
      tok.reset();
      return false;
    }

    auto end_of_token = static_cast<char const*>(::memchr(first, static_cast<int>(Separator), end-first));
    
    if (end_of_token) {
      tok.set(first, end_of_token-first);
      first = end_of_token + 1;
    } else {
      // separator not found ==> this is last token
      tok.set(first, end-first);
      first = end;
    }
    
    return true;
  }
private:
  
};


// Predefined tokenizer
template<char Separater>
using StringViewTokenizer = 
  boost::tokenizer<
    StringViewCharSeparator<Separater>,
    kanon::StringView::const_iterator,
    kanon::StringView>; 

typedef StringViewTokenizer<' '> StringViewSpaceTokenizer;
typedef StringViewTokenizer<','> StringViewCommaTokenizer;
typedef StringViewTokenizer<':'> StringViewColonTokenizer;

} // namespace memcache

#endif // KANON_EXAMPLE_MEMCACHE_SPACE_SEPARATOR_H
