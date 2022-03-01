#include <assert.h>

#include "../process.h"

using namespace unix;

int main()
{
  Process process{};
  assert(process.Fork(
    []()
    {
      printf("parent process\n");
    },
    []()
    {
      printf("child process\n");
    }));
}