#include "kanon/net/InetAddr.h"

#include <gtest/gtest.h>

using namespace kanon;

TEST(InetAddr, resolve) {
    std::vector<InetAddr> addrs;
    InetAddr::resolve("www.baidu.com", addrs);

    for (auto& addr : addrs) {
        std::cout << addr.toIpPort() << '\n';
    }

    addrs.clear();

    InetAddr::resolve("conzxy", addrs);

    for (auto& addr : addrs) {
        std::cout << addr.toIpPort() << '\n';
    }

}

int main(int argc, char* argv[]) {
    testing::InitGoogleTest();

    return RUN_ALL_TESTS();
}