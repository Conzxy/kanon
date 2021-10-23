#include "kanon/net/EventLoop.h"
#include "kanon/log/LogFile.h"

using namespace kanon;

void print() KANON_NOEXCEPT {
    puts("print");
}

int main(int argc, char* argv[]) {
    EventLoop loop;

    //LogFile<> log_file(::basename(argv[0]), 20000000);

    //Logger::setFlushCallback([&log_file](){
        //log_file.flush();
    //});

    //Logger::setOutputCallback([&log_file](char const* data, size_t num){
        //log_file.append(data, num);
    //});

    auto id1 = loop.runAfter(&print, 1); KANON_UNUSED(id1);
    auto id2 = loop.runEvery([](){
		LOG_INFO << "runEvery() 1 second";
    }, 1); KANON_UNUSED(id2);

    loop.loop();

}
