#include <string>
#include <iostream>

#include "kanon/net/user_client.h"
#include "kanon/util/ptr.h"
#include <kanon/rpc/krpc_client.h>

#include "echo.pb.h"

using namespace kanon;
using namespace kanon::protobuf::rpc;

void Done(KRpcClient* cli, EchoReply* reply) {
  LOG_INFO << "reply: " << reply->msg();
  std::cout << "Type a message: ";
  std::cout.flush();
}

int main() {
  EventLoopThread thr_loop;
  
  auto loop = thr_loop.StartRun();

  KRpcClient cli(loop, InetAddr("127.0.0.1:9998"), "Echo Client");

  cli.Connect();

  auto stub = cli.GetStub<EchoService::Stub>();
  
  EchoArgs args;
  EchoReply reply; 
  std::string msg;
  
  std::cout << "Type a message: ";
  while (std::cin >> msg) {
    args.set_msg(msg);
    stub->Echo(NULL, &args, &reply, PROTOBUF::NewCallback(&Done, &cli, &reply));
  }
  
  cli.Disconnect();
}
