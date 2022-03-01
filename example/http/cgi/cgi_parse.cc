#include <string>
#include <unordered_map>

#include "cgi_parse.h"

using namespace kanon;

namespace cgi {

QueryMap ParseQueryString(StringView query)
{
  KANON_ASSERT(query.front() == '?', "The first character in query string must be ?");
  query.remove_prefix(1);

  StringView::size_type equal_pos = StringView::npos;
  StringView::size_type and_pos = StringView::npos;
  std::string key;
  std::string val;

  QueryMap kvs;

  for (;;) {
    equal_pos = query.find('=');
    and_pos = query.find('&');

    // and_pos == npos is also ok
    kvs.emplace(
      query.substr_range(0, equal_pos).ToString(),
      query.substr_range(equal_pos+1, and_pos).ToString());

    if (and_pos == StringView::npos) break;

    query.remove_prefix(and_pos+1);
  }

  return kvs;
}

} // namespace cgi