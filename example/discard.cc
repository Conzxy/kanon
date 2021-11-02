#include "kanon/log/AsyncLogTrigger.h"

#include "kanon/net/TcpServer.h"
#include "kanon/net/EventLoop.h"
#include "kanon/net/InetAddr.h"
#include "kanon/net/TcpConnection.h"
#include "kanon/net/Buffer.h"

using namespace kanon;

class DiscardServer : noncopyable {
public:
    DiscardServer() 
        : DiscardServer{ InetAddr{ 9999 } }
    {
        //server_.setLoopNum(3);
    }

    explicit DiscardServer(InetAddr const& listenAddr)
        : server_{ &loop_, listenAddr, "Discard Server" }
    {
        server_.setMessageCallback([](
            TcpConnectionPtr const& conn,
            Buffer& buffer,
            TimeStamp stamp) 
        {
            KANON_UNUSED(stamp);
            auto content = buffer.toStringView();
            LOG_INFO << "Recv length: " << content.size() << "; content: " << content.data();
            buffer.advance(content.size());
            LOG_INFO << "But this content will be discarded";
            conn->send("discard");    
        });
    }

    void start() {
        //server_.setLoopNum(100);
        server_.start();
        loop_.loop();
    }

private:
    EventLoop loop_;

    TcpServer server_;
};


int main(int argc, char* argv[]) {
    //AsyncLogTrigger dummy(::basename(argv[0]), 20000);
    DiscardServer server{ };

    server.start();
}
