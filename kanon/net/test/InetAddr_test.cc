#include "kanon/net/inet_addr.h"

#include <gtest/gtest.h>

using namespace kanon;

TEST(InetAddr, resolve) {
    std::vector<InetAddr> addrs;
    InetAddr::Resolve("www.baidu.com", addrs);

    for (auto& addr : addrs) {
        std::cout << addr.ToIpPort() << '\n';
    }

    addrs.clear();

    InetAddr::Resolve("conzxy", addrs);

    for (auto& addr : addrs) {
        std::cout << addr.ToIpPort() << '\n';
    }

}

int main(int argc, char* argv[]) {
    testing::InitGoogleTest();

    return RUN_ALL_TESTS();
}