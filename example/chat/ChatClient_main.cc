#include "ChatClient.h"

#include "kanon/net/EventLoopThread.h"
#include "kanon/log/AsyncLogTrigger.h"

#include <iostream>

using namespace kanon;
using namespace std;

int main(int , char* argv[]) {
  AsyncLogTrigger::instance(::basename(argv[0]), 20000, "/root/.log/");

  EventLoopThread loop_thread{ };
  InetAddr serv_addr{ "127.0.0.1", 9999 };

  ChatClient client{ loop_thread.start(), serv_addr };

  client.connect();
  
  string line; 
  
  while (true) {
    cout << "Please type a message: ";

    if (std::getline(std::cin, line)) {
      if (std::cin.eof()) 
        break;
      client.writeMessage(line);
    }
  }
  client.disconnect();

}
