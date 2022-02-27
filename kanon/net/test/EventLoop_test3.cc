#include "kanon/net/event_loop.h"
#include "kanon/log/log_file.h"

using namespace kanon;

void print() noexcept {
    puts("print");
}

int main(int argc, char* argv[]) {
    EventLoop loop;

    //LogFile<> log_file(::basename(argv[0]), 20000000);

    //Logger::SetFlushCallback([&log_file](){
        //log_file.flush();
    //});

    //Logger::SetOutputCallback([&log_file](char const* data, size_t num){
        //log_file.Append(data, num);
    //});

    auto id1 = loop.RunAfter(&print, 1); KANON_UNUSED(id1);
    auto id2 = loop.RunEvery([](){
    LOG_INFO << "RunEvery() 1 second";
    }, 1); KANON_UNUSED(id2);

    loop.StartLoop();

}
