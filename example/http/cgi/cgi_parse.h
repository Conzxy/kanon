#include <string>
#include <unordered_map>

#include "kanon/string/string_view.h"

namespace cgi {

using QueryMap = std::unordered_map<std::string, std::string>;

QueryMap ParseQueryString(kanon::StringView query);

}