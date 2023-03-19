#include "kanon/log/logger.h"

#include "kanon/net/tcp_server.h"
#include "kanon/net/event_loop.h"
#include "kanon/net/inet_addr.h"
#include "kanon/net/tcp_connection.h"
#include "kanon/net/buffer.h"

using namespace kanon;

class DiscardServer : public TcpServer {
public:
	explicit DiscardServer(EventLoop& loop)
		: TcpServer(&loop, InetAddr(9999), "Discard Server")
	{
		SetMessageCallback([](
			TcpConnectionPtr const& conn,
			Buffer& buffer,
			TimeStamp stamp) 
		{
			KANON_UNUSED(stamp);
			auto content = buffer.ToStringView();
			FMT_LOG_INFO("Recv length: % ; content: %", content.size(), content.data());
			buffer.AdvanceRead(content.size());
			LOG_INFO << "But this content will be discarded";
			conn->Send("discard\n");
		});

	}
};


int main() {
	//AsyncLogTrigger dummy(::basename(argv[0]), 20000);
	EventLoop loop;
	DiscardServer server(loop);

	server.StartRun();
	loop.StartLoop();
}
