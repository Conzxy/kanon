#include <assert.h>
#include <string.h>

#include "pipe.h"
#include "process.h"

using namespace unix;

int main()
{
  try {
    Pipe pipe;

    auto parent_func = 
      [&pipe]()
      {
        pipe.CloseReadEnd();

        pipe.Write("parent send to child\n");
      };

    auto child_func = 
      [&pipe]()
      {
        pipe.CloseWriteEnd();
        char buf[4096];

        memset(buf, 0, sizeof buf);
        pipe.Read(buf);
        ::puts(buf);
      };


    Process process{};
    assert(process.Fork(parent_func, child_func));
  } catch (std::exception const& ex) {
    perror(ex.what());
    abort();
  }
}