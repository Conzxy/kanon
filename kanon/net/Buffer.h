#ifndef KANON_NET_BUFFER_H
#define KANON_NET_BUFFER_H

#include "kanon/util/macro.h"
#include "kanon/string/string_view.h"
#include "kanon/net/endian_api.h"

#include <stdint.h>
#include <vector>

namespace kanon {

#define BUFFER_INITSIZE 1024
#define BUFFER_PREFIX_SIZE 8

/**
 * @class Buffer
 * @brief 
 * Simple continuous buffer. \n
 * It provide for byte stream with operation sucha as append and prepend. \n
 * Buffer layout:  \n
 * | prepend region | readable region | writable region | \n
 *   - prepend region ensure 8 byte at least to let user fill packet size but no need to move space. \n
 *   - readable region is where the data is stored. \n
 *   - writable region is where data to append. \n
 * @warning the buffer don't actively shrink capacity.
 */
class Buffer {
  typedef std::vector<char> data_type;
public:
  typedef data_type::size_type size_type;
  typedef data_type::value_type value_type;
  typedef data_type::pointer pointer;
  typedef data_type::const_pointer const_pointer;

  explicit Buffer(size_type init_size = BUFFER_INITSIZE)
    : read_index_{ BUFFER_PREFIX_SIZE }
    , write_index_{ BUFFER_PREFIX_SIZE }
  {
    data_.resize(BUFFER_PREFIX_SIZE + init_size);

    static_assert(BUFFER_PREFIX_SIZE == 8, "Buffer prefix size must be 8");
    assert(readable_size() == 0 && "Buffer init readable_size must be 0");
    assert(writable_size() == init_size && "Buffer init writable_size must be init_size");    
  }

  ~Buffer() = default;
  
  StringView toStringView() const KANON_NOEXCEPT
  { return StringView{ data_.data() + read_index_, static_cast<StringView::size_type>(readable_size()) }; }  

  // peek: you can understand just read but don't remove it
  // normally, this word is understanded to "secret look"
  const_pointer peek() const KANON_NOEXCEPT
  { return data_.data() + read_index_; }
  
  pointer peek() KANON_NOEXCEPT
  { return data_.data() + read_index_; }

  // You can use it in resolving HTTP request or other case
  bool findCRLF(StringView& buffer) KANON_NOEXCEPT {
    auto tmp = toStringView();
    StringView::size_type idx = 0; 

    if ((idx = tmp.find("\r\n")) == StringView::npos) {
      return false;
    } 

    buffer = tmp.substr(0, idx);
    return true;
  }

  bool findLF(StringView& buffer) KANON_NOEXCEPT {
    auto tmp = toStringView();
    StringView::size_type idx = 0; 

    if ((idx = tmp.find("\n")) == StringView::npos) {
      return false;
    } 

    buffer = tmp.substr(0, idx);
    return true;
  }
  
  bool findCRLF(std::string& str) {
    StringView tmp;
    if (findCRLF(tmp) == true) {
      str = tmp.toString();
      advance(tmp.size());
      return true;
    }

    return false;
  }

  bool findLF(std::string& str) {
    StringView tmp;
    if (findLF(tmp) == true) {
      str = tmp.toString();
      advance(tmp.size());
      return true;
    }

    return false;
  }

  // append operation:
  void append(StringView str) {
    make_space(str.size());
    
    std::copy(str.begin(), str.end(), offset(write_index_));
    write_index_ += str.size();
  }
  
  void append(char const* str, size_t len) {
    append(StringView(str, len));
  }  
  
  void append(void const* p, size_t len) {
    append(static_cast<char const*>(p), len);
  }  

  void append64(uint64_t i) {
    auto ni = sock::toNetworkByteOrder64(i);
    append(&ni, sizeof ni);
  }  
  
  void append32(uint32_t i) {
    auto ni = sock::toNetworkByteOrder32(i);
    append(&ni, sizeof ni);
  }

  void append16(uint16_t i) {
    auto ni = sock::toNetworkByteOrder16(i);
    append(&ni, sizeof ni);
  }
  
  // prepend operation:
  void prepend(char const* str, size_t len) {
    assert(read_index_ >= BUFFER_PREFIX_SIZE); // First prepend
    assert(len <= read_index_);

    read_index_ -= len;
    ::memcpy(&*offset(read_index_), str, len);
  }
  
  void prepend(void const* str, size_t len) {
    prepend(static_cast<char const*>(str), len);
  }

  void prepend16(uint16_t i) {
    auto ni = sock::toNetworkByteOrder16(i);
    prepend(&ni, sizeof ni);  
  }

  void prepend32(uint32_t i) {
    auto ni = sock::toNetworkByteOrder32(i);
    prepend(&ni, sizeof ni);  
  }

  void prepend64(uint64_t i) {
    auto ni = sock::toNetworkByteOrder64(i);
    prepend(&ni, sizeof ni);  
  }

  // peek operation:
  uint16_t peek16() const KANON_NOEXCEPT {
    uint16_t i;
    ::memcpy(&i, peek(), sizeof i);
      
    return sock::toHostByteOrder16(i);
  }

  uint32_t peek32() const KANON_NOEXCEPT {
    uint32_t i;
    ::memcpy(&i, peek(), sizeof i);
      
    return sock::toHostByteOrder32(i);
  }

  uint64_t peek64() const KANON_NOEXCEPT {
    uint64_t i;
    ::memcpy(&i, peek(), sizeof i);
      
    return sock::toHostByteOrder64(i);
  }
  
  // advance operation:
  void advance(size_type n) KANON_NOEXCEPT {
    assert(n <= readable_size());

    // Reset read and write index to
    // make writeable_size() larger, 
    // then reduce the number of the call to makeSpace()
    // (Although it also reset index in makeSpace()
    // but now is a good chance to reset).
    if (n == readable_size()) {
      read_index_ = write_index_ = BUFFER_PREFIX_SIZE;
    } else {
      read_index_ += n;
    }

  }  
  
  // since uint64_t in some machine which only support 32bit at most 
  // present 32bit, you can't just write advance(8)  
  void advance64() KANON_NOEXCEPT
  { advance(sizeof(uint64_t)); }  

  void advance32() KANON_NOEXCEPT
  { advance(sizeof(uint32_t)); }

  void advance16() KANON_NOEXCEPT
  { advance(sizeof(uint16_t)); }
  
  // retrieve  operation:
  std::string retrieveAllAsString() {
    return retrieveAsString(readable_size());
  }

  std::string retrieveAsString(size_type len) {
    assert(len <= readable_size());
    std::string str(peek(), len);
    advance(len);
    return str;
  }
  
  // read operation:
  uint16_t read16() KANON_NOEXCEPT {
    auto ret = peek16();
    advance16();
    return ret;
  }
  
  uint32_t read32() KANON_NOEXCEPT {
    auto ret = peek32();
    advance32();
    return ret;
  }

  uint64_t read64() KANON_NOEXCEPT {
    auto ret = peek64();
    advance64();
    return ret;
  }
  
  // field infomation:
  data_type const& data() const KANON_NOEXCEPT
  { return data_; }
  
  size_type prependable_size() const KANON_NOEXCEPT
  { return read_index_; }

  size_type readable_size() const KANON_NOEXCEPT
  { return write_index_ - read_index_; }
  
  size_type writable_size() const KANON_NOEXCEPT
  { return data_.size() - write_index_; }
  
  size_type capacity() const KANON_NOEXCEPT
  { return data_.capacity(); }

  void shrink(size_type n);

  void swap(Buffer& other) KANON_NOEXCEPT {
    std::swap(write_index_, other.write_index_);
    std::swap(read_index_, other.read_index_);
    std::swap(data_, other.data_);
  }
  
  // set saved_errno to avoid import Logger.h to Buffer.cc  
  size_type readFd(int fd, int& saved_errno);  
private:
  typedef data_type::const_iterator const_iterator;
  typedef data_type::iterator iterator;
  
  void make_space(size_type len) {
    // if all unused space size < len + prefix size
    // only to expand buffer
    // else we can resize inside buffer through adjust layout
    // | prepend size | readable size | writable size | unused |
    //            ||
    // | prefix size | readable size | writable size | unused |
    if (len + BUFFER_PREFIX_SIZE >  prependable_size() + writable_size()) {
      // just fit
      data_.resize(write_index_ + len);

      if (prependable_size() < BUFFER_PREFIX_SIZE) {
        // It should be discarded
        auto r = read_index_ + BUFFER_PREFIX_SIZE - prependable_size(); 
        std::copy(offset(r), offset(r+readable_size()), 
            offset(BUFFER_PREFIX_SIZE));
      }
    } else {
      // reuse unused space between read_index_ and writable_index
      //
      // read_index_ should after prefix size
      // i.e. response size should prepend at last
      assert(BUFFER_PREFIX_SIZE <= read_index_);
      auto readable = this->readable_size();

      std::copy(offset(read_index_),
            offset(write_index_),
            offset(BUFFER_PREFIX_SIZE));
      read_index_ = BUFFER_PREFIX_SIZE;
      write_index_ = read_index_ + readable;
      
      assert(readable == this->readable_size());

    }

  }
  
  const_iterator offset(size_type n) const KANON_NOEXCEPT
  { return data_.cbegin() + n; }

  iterator offset(size_type n) KANON_NOEXCEPT
  { return data_.begin() + n; }

  size_type read_index_;
  size_type write_index_;
  data_type data_;
};

} // namespace kanon

#endif // KANON_NET_BUFFER_H
