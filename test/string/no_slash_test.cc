#include "kanon/string/string_view.h"

#include <iostream>

using namespace kanon;

StringView RmSlash(StringView fullname)
{
  StringView::size_type pos;
  
  while ((pos = fullname.find('/')) != StringView::npos)
  {
    std::cout << "pos = " << pos << '\n';
      fullname.remove_prefix(pos + 1);
      std::cout << fullname.data() << '\n';
  }

  return fullname;
}

int main()
{
  std::cout << (int)StringView::npos;
  std::cout << RmSlash("~/kanon/kanon").data();
}
