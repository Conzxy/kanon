#include "MemcacheServer.h"

#include "kanon/net/common.h"

using namespace kanon;

namespace memcache {

MemcacheServer::MemcacheServer(
    EventLoop* loop,
    Options const& options)
  : server_(loop, InetAddr(options.tcpPort), "MemcacheServer") {
    server_.setLoopNum(options.threadNum);
    
    server_.setConnectionCallback(
      [this](TcpConnectionPtr const& conn) {
        if (conn->isConnected()) {
          // In c++20, you can just write:
          // assert(sessions_.contains(conn->name()));
          assert(sessions_.find(conn->name()) == sessions_.end());
          sessions_.emplace(
            std::piecewise_construct,
            std::forward_as_tuple(conn->name()),
            std::forward_as_tuple(this, conn));
        } else {
          assert(sessions_.find(conn->name()) != sessions_.end());
          sessions_.erase(conn->name());
        }
      });
}

bool MemcacheServer::storeItem(
    ItemConstPtr const& item,
    Session::CommandType command,
    bool& exists) {
  ItemPtr _item(std::const_pointer_cast<Item>(item));
  const auto index = item->hash() % kHashSetSize;
  auto& items = sets_[index].items_;
  auto& lock = sets_[index].lock_;

  auto iter = items.find(_item);
  auto const& old_item = *iter;
  exists = (iter != items.end()); 
  
  // TODO narrow the critic section
  MutexGuard guard(lock); 

  switch (command) {
  case Session::kAdd:
    // The item must be not exists in items
    if (exists) {
      return false;
    } else {
      _item->incrCas();
      auto success = items.emplace(std::move(_item));KANON_UNUSED(success);
      assert(success.second == true);
      return true;
    }
    break;
  case Session::kSet:{
    // store this data
    // if already exists, erase it first.
    _item->incrCas();
    if (exists) {
      auto num = items.erase(_item);KANON_UNUSED(num);
      assert(num == 1);
    }
      
    auto success = items.emplace(std::move(_item));KANON_UNUSED(success);
    assert(success.second == true);}
    break;
  case Session::kReplace:
    // store this data
    // but only when server already hold this data
    if (exists) {
      auto num = items.erase(_item);KANON_UNUSED(num);
      assert(num == 1);

      _item->incrCas();
      auto success = items.emplace(std::move(_item));KANON_UNUSED(success);
      assert(success.second == true);
    } else {
      return false;
    }
    break;
  case Session::kPrepend:
  case Session::kAppend:
    // Don't accept flag and exptime
    // item must be exists
    if (exists) {
      auto new_item = kanon::make_unique<Item>(
          _item->key(),
          _item->valueLen() + old_item->valueLen(),
          old_item->flag(),
          old_item->expirationTime());

      new_item->incrCas();
      
      // 2 indicate \r\n
      if (command == Session::kPrepend) {
        new_item->append(_item->valueData(), _item->valueLen()-2);
        new_item->append(old_item->value());
      } else {
        new_item->append(old_item->valueData(), old_item->valueLen()-2);
        new_item->append(_item->value());
      }

      auto num = items.erase(old_item); KANON_UNUSED(num);
      assert(num == 1);

      auto success = items.emplace(std::move(new_item)); KANON_UNUSED(success);
      assert(success.second);
    } else {
      return false;
    }

    break;
  case Session::kCas:
    // Since cas unique value must be acquired by "get" command,
    // if not exists or cas not match, we can think it must be modified
    // by others
    if (exists && _item->cas() == old_item->cas()) {
      auto num = items.erase(old_item); KANON_UNUSED(num);
      assert(num == 1);
      
      _item->incrCas();
      auto success = items.emplace(std::move(_item)); KANON_UNUSED(success);
      assert(success.second);
    } else {
      return false;
    }
    break;
  default:
    // FIXME send message to client
    assert(false);
  } 

  return true;
}

bool MemcacheServer::deleteItem(const ItemConstPtr &item) {

  return true;
}

bool MemcacheServer::getItem(const ItemConstPtr &item) {
  return true;
}

} // memcache
