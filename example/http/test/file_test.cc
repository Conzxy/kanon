#include "file.h"

#include <string.h>

using namespace kanon;
using namespace http;

int main()
{
  File fd("/root/kanon/example/http/html/kanon2.jpg", File::kRead);

  char buf[1 << 16];

  size_t total = 0;
  size_t cnt = 0;
  for (size_t readn = fd.Read(buf); readn != (size_t)-1; readn = fd.Read(buf)) {
    cnt++;
    total += readn;
    printf("%s", buf);
    memset(buf, 0, sizeof buf);
    if (fd.IsEof()) break;
  }

  printf("\ntotal: %lu, cnt = %lu\n", total, cnt);
}