#include "kanon/net/inet_addr.h"

#include <gtest/gtest.h>

using namespace kanon;

TEST(InetAddr, resolve) {
  auto addrs = InetAddr::Resolve("www.baidu.com", "http");

  for (auto& addr : addrs) {
    std::cout << addr.ToIpPort() << '\n';
  }

  std::cout << "=====" << "Client resolve" << "=====\n";
  addrs = InetAddr::Resolve("www.baidu.com", "http");

  for (auto& addr : addrs) {
    std::cout << addr.ToIpPort() << "\n";
  }

  std::cout << "=====" << "Server resolve" << "=====\n";
  addrs = InetAddr::Resolve("www.baidu.com", "80", InetAddr::kServer);

  for (auto& addr : addrs) {
    std::cout << addr.ToIpPort() << "\n";
  }
}

TEST(InetAddr, ctor) {
  InetAddr addr("www.baidu.com", "http");

  std::cout << addr.ToIpPort() << "\n";

  InetAddr addr2("127.0.0.1:80");

  std::cout << addr2.ToIpPort() << "\n";

  InetAddr addr3("[3ff2::0:8:1fff]:80");

  std::cout << addr3.ToIpPort() << "\n";

  InetAddr addr4("www.baidu.com:80");

  std::cout << addr4.ToIpPort() << "\n";
}

int main(int argc, char* argv[]) {
    testing::InitGoogleTest();

    return RUN_ALL_TESTS();
}