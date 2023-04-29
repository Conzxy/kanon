#include "kanon/protobuf/protobuf_codec2.h"
#include "kanon/net/user_client.h"

#include "pb/echo.pb.h"

using namespace kanon;
using namespace kanon::protobuf;

int main()
{
  EventLoop loop;
  auto client = kanon::NewTcpClient(&loop, InetAddr("127.0.0.1:9999"));

  ProtobufCodec2 codec(StringView("Echo", 4), 1 << 16);

  client->SetConnectionCallback([&codec](TcpConnectionPtr const &conn) {
    if (conn->IsConnected()) {
      codec.SetUpConnection(conn);
      EchoArgs args;
      args.set_msg("ProtobufCodec2 test");

      codec.Send(conn, &args);
    } else {
      exit(0);
    }
  });

  codec.SetMessageCallback([&client](TcpConnectionPtr const &conn,
                                     Buffer &buffer, size_t payload_size,
                                     TimeStamp) {
    EchoReply reply;
    ParseFromBuffer(&reply, payload_size, &buffer);

    LOG_INFO << "The cached size after parse = " << reply.GetCachedSize();
    LOG_INFO << "The payload size = " << payload_size;
    LOG_INFO << "msg: " << reply.msg();

    (void)conn;
    client->Disconnect();
  });

  client->Connect();

  loop.StartLoop();
}
