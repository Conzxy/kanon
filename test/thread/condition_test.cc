#include "kanon/thread/condition.h"

using namespace kanon;

int main() {
  MutexLock lock;
  Condition cond(lock);

  cond.WaitForSeconds(5);

}