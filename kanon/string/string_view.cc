#include "kanon/string/string_view.h"

namespace kanon {

std::vector<std::string> StringView::split(StringView spliter) const 
{
  size_type indicator_begin = 0;
  size_type indicator_end = find(spliter);
  std::vector<std::string> ret{};
  std::string tmp;    

  while (true) {
    if (indicator_end == npos)
      indicator_end = len_;
    auto str = substr(indicator_begin, indicator_end - indicator_begin);
    tmp.assign(str.data_, str.len_);
    ret.emplace_back(std::move(tmp));

    if (indicator_end == len_) break;
    indicator_begin = indicator_end + spliter.size();
    indicator_end = find(spliter, indicator_begin);
  }
  
  return ret;
}

constexpr StringView::size_type StringView::npos;

} // namespace kanon
