#ifndef KANON_EXAMPLE_MEMCACHE_SESSION_H
#define KANON_EXAMPLE_MEMCACHE_SESSION_H

#include "Item.h"
#include "StringViewTokenizer.h"

#include "kanon/util/optional.h"
#include "kanon/util/noncopyable.h"
#include "kanon/net/callback.h"
#include "kanon/util/unique_ptr.h"
#include "kanon/net/Buffer.h"

namespace memcache {

class MemcacheServer;

/**
 * @class Session
 * @brief 
 * Parse request, including command type, key, flags
 * and so on. Determine do what work and delege to 
 * server.
 */
class Session : kanon::noncopyable {
public:
  typedef StringViewSpaceTokenizer Tokenizer;

  enum Phase {
    kNewCommand = 0,
    kReceiveValue,
    kDiscardValue,
  };
  
  enum CommandType {
    kSet = 0,
    kAdd,
    kReplace,
    kDelete,
    kPrepend,
    kAppend,
    kGet,
    kGets,
    kCas,
    kInvalid,
  };

  Session(MemcacheServer* server, TcpConnectionPtr const& conn);

private:
  void parseRequestLine(kanon::Buffer& buffer); 

  /**
   * @param request complete request line
   * @return 
   * indicate process of the request if has finished
   */
  bool processRequest(kanon::StringView request);
  
  // receive phase 
  void receiveValue(kanon::Buffer& buffer);
  // discard phase
  void discardValue(kanon::Buffer& buffer);
  
  // specific work 
  /**
   * @param beg begin iterator of request after command
   * @param end end iterator of request
   */
  void doDelete(Tokenizer::iterator beg,   Tokenizer::iterator end);
  /**
   * @see doDelete
   */
  bool doStorage(Tokenizer::iterator beg,  Tokenizer::iterator end);
  /**
   * @see doStorage
   */
  void doRetrieve(Tokenizer::iterator beg, Tokenizer::iterator end);
  
  // send message and do some handling
  void reply(kanon::StringView msg);
  void clientError(kanon::StringView msg);
  bool formatError(Tokenizer::iterator beg, Tokenizer::iterator end, kanon::StringView msg);

  kanon::optional<kanon::StringView>
  checkKeyValid(Tokenizer::iterator beg, Tokenizer::iterator end);
  
  // Maintained by Session instead of Item
  // !No need plus 2, since valueLen() contains.
  u32 needBytes(Item* item) const KANON_NOEXCEPT {
    return item->valueLen() - hasReceiveBytes_;
  }
  
  // When process of a request has finished,
  // reset all state variable about this request.
  void resetRequestParam() KANON_NOEXCEPT;



  kanon::TcpConnectionPtr conn_;

  MemcacheServer* server_;  
  CommandType command_;
  bool noreply_;

  Phase phase_;

  // Use this and totalLen() for Item, we can
  // compute the bytes that we need read
  //
  // !It is no need let this be a data member 
  // of Item, since the number of Item likely 
  // over Session(fix the muduo not do)
  u32 hasReceiveBytes_;

  // If client send data block which over 1GB,
  // we just discard it all, don't accept any byte.
  u64 discardBytes_;
  
  // Since May be in multithread environment,
  // If client send "delete" command to erase 
  // key-value pair, and the value is only once
  // and session is managed by server, so it likely
  // call send(char const*, usize) in loop, it is 
  // asynchronous, we must use outputBuffer_ to cache
  // value block, call send(Buffer&).
  kanon::Buffer outputBuffer_; 
  
  // Present current Item
  // Used for storage command
  std::unique_ptr<Item> currItem_;
  
  // Cache Item
  // Used for delete, retrieve command
  // Prevent this dead early
  std::shared_ptr<Item> cacheItem_;

  static constexpr u8 kMaxKeyLength = 250;
  static constexpr usize kMaxBytes = 1 << 30;

};

} // namespace memcache

#endif // KANON_EXAMPLE_MEMCACHE_SESSION_H
