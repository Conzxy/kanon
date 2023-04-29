#include "kanon/net/user_server.h"

#include "kanon/protobuf/protobuf_codec2.h"
#include "pb/echo.pb.h"

using namespace kanon;
using namespace kanon::protobuf;

struct Session {};

int main()
{
  EventLoop loop;
  TcpServer server(&loop, InetAddr(9999), "EchoProtbufCodec2 test");

  ProtobufCodec2 codec(StringView("Echo"), 1 << 16);
  codec.SetMessageCallback([&codec](TcpConnectionPtr const &conn,
                                    Buffer &buffer, size_t payload_size,
                                    TimeStamp) {
    EchoArgs args;
    ParseFromBuffer(&args, payload_size, &buffer);

    EchoReply reply;
    reply.set_msg(std::move(args.msg()));

    codec.Send(conn, &reply);
  });

  server.SetConnectionCallback([&codec](TcpConnectionPtr const &conn) {
    if (conn->IsConnected()) {
      codec.SetUpConnection(conn);
      auto session = new Session();
      LOG_DEBUG << "session: " << session;
      conn->SetContext(*session);
    } else {
      auto session = AnyCast2<Session *>(conn->GetContext());
      LOG_DEBUG << "session: " << session;
      delete session;
    }
  });

  server.StartRun();
  loop.StartLoop();
}
