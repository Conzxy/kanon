#include "kanon/thread/rw_lock.h"

#include <unordered_map>

#include <gtest/gtest.h>

#include "kanon/log/logger.h"
#include "kanon/thread/thread.h"

class ConcurrentHashTable {
public:
  ConcurrentHashTable() = default;
  ~ConcurrentHashTable() = default;

  void Put(int val)
  {
    LOG_DEBUG << "Put " << val;
    kanon::WLockGuard g(lock_);
    table_.emplace(val, val);
  }

  void Remove(int val)
  {
    kanon::WLockGuard g(lock_);
    table_.erase(val);
  }

  std::pair<bool, int> Search(int val)
  {
    LOG_DEBUG << "Search " << val;
    kanon::RLockGuard g(lock_);
    auto iter = table_.find(val);

    if (iter == table_.end()) {
      return std::make_pair(false, 0);
    }

    return std::make_pair(true, iter->second);
  }
private:
  std::unordered_map<int, int> table_;
  kanon::RWLock lock_;
};

ConcurrentHashTable g_table;

static constexpr int NUM = 10000;

TEST(rw_lock, concurrent_hash_table) {
  kanon::Thread thr([]() {
    LOG_DEBUG << "Thread1";
    for (int i = 0; i < NUM; ++i)
      g_table.Put(i);
    LOG_DEBUG << "Thread1(end)";
  });

  kanon::Thread thr2([]() {
    LOG_DEBUG << "Thread2";
    for (int i = NUM / 2; i < NUM; ++i) {
      g_table.Put(i);
    }
    LOG_DEBUG << "Thread2(end)";
  });


  kanon::Thread thr3([]() {
      LOG_DEBUG << "Thread3";
      for (int i = 0; i < NUM; ++i) {
        g_table.Search(i);
      }
      LOG_DEBUG << "Thread3(end)";
    });

  thr.StartRun();
  thr2.StartRun();
  thr3.StartRun();

  thr.Join();
  thr2.Join();
  thr3.Join();

  for (int i = 0; i < NUM; ++i) {
    auto res = g_table.Search(i);

    EXPECT_EQ(res.first, true);

    EXPECT_EQ(res.second, i);
  }

}

int main()
{
  testing::InitGoogleTest();

  return RUN_ALL_TESTS();
}