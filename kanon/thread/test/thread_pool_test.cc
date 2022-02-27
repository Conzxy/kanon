#include "kanon/thread/thread_pool.h"

#include "kanon/log/logger.h"

using namespace kanon;

void print() noexcept {
    LOG_INFO << "print";
}

int main() {
    ThreadPool pool{ };
    pool.StartRun(10);

    for (int i = 0; i != 10; ++i)
        pool.Push(print);

}