#include "TcpServer.h"
#include "kanon/net/InetAddr.h"
#include "kanon/net/TcpConnection.h"
#include "kanon/net/EventLoop.h"

using namespace kanon;

TcpServer::TcpServer(EventLoop* loop,
					 InetAddr const& listen_addr,
					 StringArg name,
					 bool reuseport)
	: loop_{ loop }
	, ip_port_{ listen_addr.toIpPort() }
	, name_{ name }
	, acceptor_{ loop_, listen_addr, reuseport }
	, next_conn_id{ 1 }

{
	acceptor_.setNewConnectionCallback([this](int cli_sock, InetAddr const& cli_addr) {
		// ensure in main thread
		loop_->assertInThread();
		
		// use eventloop pool and robin-round to choose a IO loop
		//
		char buf[64];
		::snprintf(buf, sizeof buf, "-%s#%u", ip_port_.c_str(), next_conn_id);
		++next_conn_id;
		auto conn_name = name_ + buf;
		
		auto local_addr = sock::getLocalAddr(cli_sock);
		auto conn = std::make_shared<TcpConnection>(loop_,  // io loop
													conn_name,
													cli_sock,
													local_addr,
													cli_addr);	

		connections_[conn_name] = conn;

		conn->setMessageCallback(message_callback_);
		conn->setConnectionCallback(connection_callback_);
		conn->setWriteCompleteCallback(write_complete_callback_);
		conn->setCloseCallback([this](TcpConnectionPtr const& conn) {
			// remove connection in server thread
			loop_->assertInThread();

			auto n = connections_.erase(conn->name());
			assert(n == 1);
			KANON_UNUSED(n);
			
			auto io_loop = conn->loop();
			io_loop->runInLoop([&conn]() {
					conn->connectionDestroyed();
			});
		});
	});
}

