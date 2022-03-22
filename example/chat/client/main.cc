#include "chat_client.h"

#include <iostream>

#include "kanon/net/event_loop_thread.h"
#include "kanon/log/async_log_trigger.h"

using namespace kanon;
using namespace std;

int main(int argc, char* argv[]) {
  AsyncLogTrigger::instance(::basename(argv[0]), 20000, "/root/.log/");

  EventLoopThread loop_thread{ };
  InetAddr serv_addr{ "127.0.0.1", 9999 };

  ChatClient client{ loop_thread.StartRun(), serv_addr };

  client.Connect();
  
  string line; 
  
  while (true) {
    cout << "Please type a message: ";

    if (std::getline(std::cin, line)) {
      if (std::cin.eof()) 
        break;
      client.WriteMessage(line);
    }
  }
  client.Disconnect();
}