#ifndef KANON_FILE_TRANSFER_SERVER_H
#define KANON_FILE_TRANSFER_SERVER_H

#include "kanon/net/user_server.h"

// This example just used to test the Send() behavior and ET mode
class FileTransferServer : public kanon::TcpServer {
public:
  explicit FileTransferServer(EventLoop& loop, uint16_t port);
  ~FileTransferServer();
};

#endif // KANON_FILE_TRANSFER_SERVER_H
