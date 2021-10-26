#include "kanon/thread/ThreadPool.h"

#include "kanon/log/Logger.h"

using namespace kanon;

void print() KANON_NOEXCEPT {
    LOG_INFO << "";
}

int main() {
    ThreadPool pool{ };
    pool.start(10);

    for (int i = 0; i != 10; ++i)
        pool.run(print);

}