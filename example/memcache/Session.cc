#include "Session.h"
#include "StringViewTokenizer.h"
#include "Extractor.h"
#include "Item.h"
#include "MemcacheServer.h"

#include "kanon/net/TcpServer.h"
#include "kanon/net/TcpConnection.h"

#include <assert.h>

IMPORT_NAMESPACE( kanon );

namespace memcache {

Session::Session(MemcacheServer* server, TcpConnectionPtr const& conn)
  : conn_{ conn }
  , server_{ server }
  , command_{ kInvalid }
  , noreply_{ false }
  , phase_{ kNewCommand }
  , hasReceiveBytes_{ 0 }
  , discardBytes_{ 0 } 
  , currItem_{ nullptr }
  , cacheItem_{ std::make_shared<Item>(std::string(250, 'x'), 2, 0, 0) }
{
  // !notice: 
  // Message callback is not set by server,
  // it is set by the session.
  conn->setMessageCallback(
    [this](TcpConnectionPtr const& conn,
      Buffer& buffer,
      TimeStamp receive_time) {
        KANON_UNUSED(conn);
        KANON_UNUSED(receive_time);

        switch (phase_) {
        case kNewCommand:
          parseRequestLine(buffer);
          break;
        case kReceiveValue:
          receiveValue(buffer);
          break;
        case kDiscardValue:
          discardValue(buffer);
        }
      });
}

void Session::parseRequestLine(kanon::Buffer& buffer) {
  // wait request until meet with CR&LF
  if (buffer.readable_size() > 0) {
    StringView request;
    const auto found = buffer.findCRLF(request);
    if (found) {
      if (processRequest(request)) {
        resetRequestParam();
      }

      buffer.advance(request.size() + 2);
    } else {
      if (buffer.readable_size() > 1024) {
        clientError("CLIENT_ERROR: request too long\r\n");
      }
    }
  }
}

bool Session::processRequest(StringView request) {
  assert(!noreply_ );
  assert(command_ == kInvalid);
  assert(hasReceiveBytes_ == 0);
  assert(discardBytes_ == 0);
  assert(phase_ == kNewCommand);
  assert(currItem_ == nullptr);

  // Check noreply first
  // to enable or disable the reply()
  if (request.size() >= 8) {
    auto has = request.ends_with(" noreply"); 
    if (has) {
      noreply_ = true;
      request.remove_suffix(sizeof " noreply");
    }
  }

  Tokenizer tokens(request);

  auto beg = tokens.begin();
  const auto end = tokens.end();

  formatError(beg, end, "CLIENT_ERROR: No command\r\n");
  const auto command = *beg;
 
#define SET_COMMAND_AND_FORWARD(command, handler) \
  command_ = CommandType::command; \
  handler(++beg, end);

#define SET_COMMAND_AND_FORWARD2Storage(command) \
  command_ = CommandType::command; \
  return doStorage(++beg, end);

#define SET_COMMAND_AND_FORWARD2Retrieve(command) \
  SET_COMMAND_AND_FORWARD(command, doRetrieve) 

#define SET_COMMAND_AND_FORWARD2Delete \
  SET_COMMAND_AND_FORWARD(kDelete, doDelete)

  if (command == "set") {
    // This should return false since 
    // storage command need receive data block
    SET_COMMAND_AND_FORWARD2Storage(kSet);
  } else if (command == "add") {
    SET_COMMAND_AND_FORWARD2Storage(kAdd)
  } else if (command == "replace") {
    SET_COMMAND_AND_FORWARD2Storage(kReplace)
  } else if (command == "prepend") {
    SET_COMMAND_AND_FORWARD2Storage(kPrepend)
  } else if (command == "append") {
    SET_COMMAND_AND_FORWARD2Storage(kAppend)
  } else if (command == "delete") {
    SET_COMMAND_AND_FORWARD2Delete
  } else if (command == "get") {
    SET_COMMAND_AND_FORWARD2Retrieve(kGet);
  } else if (command == "gets") {
    SET_COMMAND_AND_FORWARD2Retrieve(kGets);
  } else if (command == "cas") {
    SET_COMMAND_AND_FORWARD2Storage(kCas);
  } else {
    clientError("CLIENT_ERROR: Unknown command type\r\n");
  }

  return true;
}

void Session::receiveValue(Buffer& buffer) {
  const auto need = needBytes(getPointer(currItem_));
  const auto avali = std::min<u32>(need, buffer.readable_size());
  const auto data = buffer.toStringView();

  LOG_INFO << "data: " << data;

  currItem_->appendValue(data.substr(0, avali), hasReceiveBytes_);
  hasReceiveBytes_ += avali;
  buffer.advance(avali);

  LOG_DEBUG << "needBytes: " << needBytes(getPointer(currItem_));

  if (needBytes(getPointer(currItem_)) == 0) {

    if (currItem_->endWithCRLF()) {
      LOG_DEBUG << "receive finished";
      bool exists = false;
      if (server_->storeItem(std::move(currItem_), command_, exists)) {
        reply("STORED\r\n");
      } else {
        // operation fail
        if (command_ == kCas) {
          if (exists) {
            reply("EXISTS\r\n");
          } else {
            reply("NOT_FOUND\r\n");
          }
        } else {
          reply("NOT_STORED\r\n");
        }
      }
      
      phase_ = kNewCommand;
      resetRequestParam();
      // currItem_.reset();
    } else {
      LOG_DEBUG << "Item: (" << currItem_->key() << ", " << currItem_->value() << ")";
      clientError("CLIENT_ERROR: Bad data block\r\n");
    }
  }
}

void Session::discardValue(Buffer& buffer) {
}

bool Session::doStorage(Tokenizer::iterator beg, Tokenizer::iterator end) {
  // Format:
  // set/add/replace/prepend/append <key><flag><exptime><bytes> [noreply]\r\n
  // <data block>\r\n
  
  if (formatError(beg, end, "CLIENT_ERROR: No key\r\n"))
    return true;
  
  const auto key = *(beg++);

  if (key.size() >= kMaxKeyLength) {
    clientError("CLIENT_ERROR: Key Length too long\r\n");
    return true;
  }

  SpaceExtractor extractor(beg, end);
  
  const auto flag = extractor.extract<i64>();
  const auto exptime = extractor.extract<i32>();
  const auto bytes = extractor.extract<u64>();

  LOG_DEBUG << "flag: " << *flag;
  LOG_DEBUG << "expiration time: " << *exptime;
  LOG_DEBUG << "bytes: " << *bytes;

  bool good = flag && exptime && bytes;
  
  const auto cas_unique = extractor.extract<u64>();

  if (command_ == kCas) {
    good = good && cas_unique;
    LOG_DEBUG << "cas_unique: " << *cas_unique;
  } 
  
  if (!good) {
    clientError("CLIENT_ERROR: Format Error\r\n");
    return true;
  }



  if (*bytes > kMaxBytes) {
    phase_ = kDiscardValue;
  } else {
    // Bytes need plus 2(\r\n)
    // It simplify the message to send
    currItem_.reset((command_ == CommandType::kCas)
      ? new Item(key, *bytes+2, *flag, *exptime, *cas_unique)
      : new Item(key, *bytes+2, *flag, *exptime));

    phase_ = kReceiveValue;
  }

  return false;
}

void Session::doDelete(Tokenizer::iterator beg, Tokenizer::iterator end) {
  if (formatError(beg, end, "CLIENT_ERROR: No key\r\n"))
    return ;

  const auto key = *beg;

  if (key.size() > kMaxKeyLength) {
    clientError("CLIENT_ERROR: Key too long\r\n");
    return ;
  }
  
  cacheItem_->resetKey(key); 
  server_->deleteItem(cacheItem_);
}

void Session::doRetrieve(Tokenizer::iterator beg, Tokenizer::iterator end) {
  const auto key = checkKeyValid(beg, end);

  if (key) {
    cacheItem_->resetKey(*key);
    server_->getItem(cacheItem_);
  }
}

void Session::resetRequestParam() noexcept {
  noreply_ = false;
  command_ = kInvalid;
  hasReceiveBytes_ = 0;
  discardBytes_ = 0;
  currItem_.reset();
}

void Session::reply(kanon::StringView msg) {
  if (!noreply_) {
    conn_->send(msg);
  }
}

bool Session::formatError(Tokenizer::iterator beg, Tokenizer::iterator end, StringView msg) {
  if (beg == end) {
    clientError(msg);
    return true;
  }

  return false;
}

void Session::clientError(StringView msg) {
  if (!noreply_)
    conn_->send(msg);
  // FIXME forceClose()?
  conn_->shutdownWrite();
}

kanon::optional<kanon::StringView> 
Session::checkKeyValid(Tokenizer::iterator beg, Tokenizer::iterator end) {
  if (formatError(beg, end, "CLIENT_ERROR: No key\r\n"))
    return kanon::make_null_optional<StringView>();

  const auto key = *beg;

  if (key.size() > kMaxKeyLength) {
    clientError("CLIENT_ERROR: Key too long\r\n");
    return kanon::make_null_optional<StringView>();
  }
  
  return kanon::make_optional(key);
}

} // namespace memcache