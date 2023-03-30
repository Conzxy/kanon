#ifndef KANON_EXAMPLE_MEMCACHE_MEMCACHE_SERVER_H
#define KANON_EXAMPLE_MEMCACHE_MEMCACHE_SERVER_H

#include "kanon/net/TcpServer.h"
#include "kanon/thread/MutexLock.h"

#include "Session.h"
#include "kanon/util/macro.h"

#include <string>
#include <unordered_set>
#include <stdlib.h>
#include <memory>

namespace memcache {

/**
 * @class MemcacheServer
 * @brief 
 * It own the session(In @file Session.h)
 * and session handle the message and then the server 
 * store item, delete item and retrieve item from
 * one of these hashtables that maintained by the server.
 * 
 */
class MemcacheServer : kanon::noncopyable {
  typedef std::shared_ptr<Item> ItemPtr;
  typedef std::shared_ptr<Item const> ItemConstPtr;

public:
  struct Options {
    Options() {
      BZERO(this, sizeof(Options));
    }

    u16 threadNum; 
    u16 tcpPort;
  };

  MemcacheServer(
      kanon::EventLoop* loop,
      Options const& option);

  /**
   * @param exists Indicate item if exists in items(Used for cas operation, if cas fails, exists indicates the item modified by others instead not stored)
   * @return 
   * Indicate the operation if is successful.
   */
  bool storeItem(
      ItemConstPtr const& item,
      Session::CommandType,
      bool& exists);
  bool deleteItem(ItemConstPtr const& item);
  bool getItem(ItemConstPtr const& item);

  void start() {
    server_.start();
  }

private:
  // FIXME setContext()
  kanon::TcpServer server_;
  std::map<std::string, Session> sessions_;
  
  struct ItemHash {
    usize operator()(ItemPtr const& item) const KANON_NOEXCEPT {
      return item->hash();
    }
  };
  
  struct ItemEqual {
    bool operator()(ItemPtr const& x, ItemPtr const& y) const KANON_NOEXCEPT {
      return x->key() == y->key();
    } 
  };
  
  typedef std::unordered_set<ItemPtr, ItemHash, ItemEqual>
    ItemHashSet;

  struct LockedHashSet {
    ItemHashSet items_;
    MutexLock lock_;
  };
  
  static constexpr u16 kHashSetSize = 1024;

  std::array<LockedHashSet, kHashSetSize> sets_;
  
};

} // namespace memcache

#endif // KANON_EXAMPLE_MEMCACHE_MEMCACHE_SERVER_H