#include "kanon/net/inet_addr.h"

#include <gtest/gtest.h>

using namespace kanon;

TEST(InetAddr, resolve) {
  auto addrs = InetAddr::Resolve("www.baidu.com", "http");

  for (auto& addr : addrs) {
    std::cout << addr.ToIpPort() << '\n';
  }

  addrs = InetAddr::Resolve("172.26.34.206", "http");

  for (auto& addr : addrs) {
    std::cout << addr.ToIpPort() << "\n";
  }
}

TEST(InetAddr, ctor) {
  InetAddr addr("www.baidu.com", "http");

  std::cout << addr.ToIpPort() << "\n";
}

int main(int argc, char* argv[]) {
    testing::InitGoogleTest();

    return RUN_ALL_TESTS();
}