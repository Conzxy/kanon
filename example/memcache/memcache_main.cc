#include "MemcacheServer.h"

#include "kanon/net/EventLoop.h"
#include <boost/program_options.hpp>

#include <iostream>


namespace po = boost::program_options;

using namespace memcache;
using namespace kanon;

bool parseCmdLine(int argc, char** argv, MemcacheServer::Options& options) {
  po::options_description od("Allowed options");

  options.tcpPort = 9999;
  options.threadNum = 4;

  od.add_options()
    ("help,h", "help about memcache")
    ("tcp-port,p", po::value<u16>(&options.tcpPort), "tcp port number(default: 9999)")
    ("threads,t", po::value<u16>(&options.threadNum), "threads number of connection handler(default: 4)");

  po::variables_map vm;
  po::store(po::parse_command_line(argc, argv, od), vm);
  po::notify(vm);

  if (vm.count("help")) {
    std::cout << od << '\n';
    return false;
  }

  return true;
}

int main(int argc, char** argv) {
  MemcacheServer::Options options;

  if (parseCmdLine(argc, argv, options)) {
    EventLoop loop;
    MemcacheServer server(&loop, options);

    server.start();

    loop.loop();
  }
  
  return 0;
}