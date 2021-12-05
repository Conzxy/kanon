#include "../StringViewTokenizer.h"

using namespace memcache;

int main() {
  char const s[] = " sss ss s ";
  kanon::StringView str(s, sizeof s - 1);
  ::printf("str len: %d\n", (int)str.size());
  StringViewSpaceTokenizer toks(str.begin(), str.end());
  
  ::printf("count: %d\n", (int)std::distance(toks.begin(), toks.end()));

  for (auto const& tok : toks) {
    ::puts(std::string(tok.data(), tok.size()).c_str());  
  }

  ::puts("boost");
  
  std::string s2 = s;
  boost::tokenizer<> token(s2.begin(), s2.end());

  for (auto const& tok : token) {
    ::puts(tok.c_str());
  }

}
