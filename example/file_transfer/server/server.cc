#include "example/file_transfer/server/server.h"

#include "kanon/net/sock_api.h"

#include "session.h"

FileTransferServer::FileTransferServer(EventLoop& loop, uint16_t port)
  : kanon::TcpServer(&loop, InetAddr(port), "FileTransferServer")
{
  SetConnectionCallback([](TcpConnectionPtr const& conn) {
    if (conn->IsConnected()) {
      conn->SetContext(new FileTransferSession(conn.get()));
    } else {
      delete *kanon::AnyCast<FileTransferSession*>(conn->GetContext());
    }
  });
}

FileTransferServer::~FileTransferServer() = default;
