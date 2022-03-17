#ifndef KANON_NET_BUFFER_H
#define KANON_NET_BUFFER_H

#include <stdint.h>
#include <vector>

#include "kanon/util/macro.h"
#include "kanon/string/string_view.h"

#include "kanon/net/endian_api.h"

namespace kanon {


/**
 * Simple continuous buffer.
 * It provide for byte stream with operation sucha as append and prepend.
 * Buffer layout:
 * | prepend region | readable region | writable region |
 *   * prepend region ensure 8 byte at least to let user fill packet size but no need to move space.
 *   * readable region is where the data is stored.
 *   * writable region is where data to append.
 * @warning the buffer don't actively shrink capacity.
 */
class Buffer {
  typedef std::vector<char> data_type;

  static constexpr int kBufferInitSize = 1024;
  static constexpr int kBufferPrefixSize = 8;
public:
  typedef data_type::size_type size_type;
  typedef data_type::value_type value_type;
  typedef data_type::pointer pointer;
  typedef data_type::const_pointer const_pointer;

  explicit Buffer(size_type init_size = kBufferInitSize);
  ~Buffer();

  StringView ToStringView() const noexcept
  { return StringView{ data_.data() + read_index_, static_cast<StringView::size_type>(GetReadableSize()) }; }  

  // peek: you can understand just read but don't remove it
  // normally, this word is understanded to "secret look"
  const_pointer GetReadBegin() const noexcept
  { return data_.data() + read_index_; }
  
  pointer GetReadBegin() noexcept
  { return data_.data() + read_index_; }
  
  pointer GetWriteBegin() noexcept
  { return data_.data() + write_index_; } 

  // You can use it in resolving HTTP request or other case
  bool FindCrLf(StringView& buffer) noexcept {
    auto tmp = ToStringView();
    StringView::size_type idx = 0; 

    if ((idx = tmp.find("\r\n")) == StringView::npos) {
      return false;
    } 

    buffer = tmp.substr(0, idx);
    return true;
  }

  bool FindLf(StringView& buffer) noexcept {
    auto tmp = ToStringView();
    StringView::size_type idx = 0; 

    if ((idx = tmp.find("\n")) == StringView::npos) {
      return false;
    } 

    buffer = tmp.substr(0, idx);
    return true;
  }
  
  bool FindCrLf(std::string& str) {
    StringView tmp;
    if (FindCrLf(tmp) == true) {
      str = tmp.ToString();
      AdvanceRead(tmp.size());
      return true;
    }

    return false;
  }

  bool FindLf(std::string& str) {
    StringView tmp;
    if (FindLf(tmp) == true) {
      str = tmp.ToString();
      AdvanceRead(tmp.size());
      return true;
    }

    return false;
  }

  // append operation:
  void Append(StringView str) {
    MakeSpace(str.size());
    
    std::copy(str.begin(), str.end(), offset(write_index_));
    write_index_ += str.size();
  }
  
  void Append(char const* str, size_t len) {
    Append(StringView(str, len));
  }  
  
  void Append(void const* p, size_t len) {
    Append(static_cast<char const*>(p), len);
  }  

  void Append64(uint64_t i) {
    auto ni = sock::ToNetworkByteOrder64(i);
    Append(&ni, sizeof ni);
  }  
  
  void Append32(uint32_t i) {
    auto ni = sock::ToNetworkByteOrder32(i);
    Append(&ni, sizeof ni);
  }

  void Append16(uint16_t i) {
    auto ni = sock::ToNetworkByteOrder16(i);
    Append(&ni, sizeof ni);
  }
  
  // prepend operation:
  void Prepend(char const* str, size_t len) {
    assert(read_index_ >= kBufferPrefixSize); // First prepend
    assert(len <= read_index_);

    read_index_ -= len;
    ::memcpy(&*offset(read_index_), str, len);
  }
  
  void Prepend(void const* str, size_t len) {
    Prepend(static_cast<char const*>(str), len);
  }

  void Prepend16(uint16_t i) {
    auto ni = sock::ToNetworkByteOrder16(i);
    Prepend(&ni, sizeof ni);  
  }

  void Prepend32(uint32_t i) {
    auto ni = sock::ToNetworkByteOrder32(i);
    Prepend(&ni, sizeof ni);  
  }

  void Prepend64(uint64_t i) {
    auto ni = sock::ToNetworkByteOrder64(i);
    Prepend(&ni, sizeof ni);  
  }

  // peek operation:
  uint16_t GetReadBegin16() const noexcept {
    uint16_t i;
    ::memcpy(&i, GetReadBegin(), sizeof i);
      
    return sock::ToHostByteOrder16(i);
  }

  uint32_t GetReadBegin32() const noexcept {
    uint32_t i;
    ::memcpy(&i, GetReadBegin(), sizeof i);
      
    return sock::ToHostByteOrder32(i);
  }

  uint64_t GetReadBegin64() const noexcept {
    uint64_t i;
    ::memcpy(&i, GetReadBegin(), sizeof i);
      
    return sock::ToHostByteOrder64(i);
  }
  
  // advance operation:
  void AdvanceRead(size_type n) noexcept {
    assert(n <= GetReadableSize());

    // Reset read and write index to
    // make GetWritableSize() larger, 
    // then reduce the number of the call to makeSpace()
    // (Although it also reset index in makeSpace()
    // but now is a good chance to reset).
    if (n == GetReadableSize()) {
      read_index_ = write_index_ = kBufferPrefixSize;
    } else {
      read_index_ += n;
    }

  }  
  
  // since uint64_t in some machine which only support 32bit at most 
  // present 32bit, you can't just write AdvanceRead(8)  
  void AdvanceRead64() noexcept
  { AdvanceRead(sizeof(uint64_t)); }  

  void AdvanceRead32() noexcept
  { AdvanceRead(sizeof(uint32_t)); }

  void AdvanceRead16() noexcept
  { AdvanceRead(sizeof(uint16_t)); }
  
  void AdvanceWrite(size_type n) noexcept {
    assert(n <= GetWritableSize());

    write_index_ += n;
  } 

  // retrieve  operation:
  std::string RetrieveAllAsString() {
    return RetrieveAsString(GetReadableSize());
  }

  std::string RetrieveAsString(size_type len) {
    assert(len <= GetReadableSize());
    std::string str(GetReadBegin(), len);
    AdvanceRead(len);
    return str;
  }
  
  // read operation:
  uint16_t Read16() noexcept {
    auto ret = GetReadBegin16();
    AdvanceRead16();
    return ret;
  }
  
  uint32_t Read32() noexcept {
    auto ret = GetReadBegin32();
    AdvanceRead32();
    return ret;
  }

  uint64_t Read64() noexcept {
    auto ret = GetReadBegin64();
    AdvanceRead64();
    return ret;
  }
  
  // field infomation:
  data_type const& GetData() const noexcept
  { return data_; }
  
  size_type GetPrependableSize() const noexcept
  { return read_index_; }

  size_type GetReadableSize() const noexcept
  { return write_index_ - read_index_; }
  
  size_type GetWritableSize() const noexcept
  { return data_.size() - write_index_; }
  
  size_type GetCapacity() const noexcept
  { return data_.capacity(); }
  
  void ReserveWriteSpace(uint32_t len) noexcept {
    if (len > GetWritableSize()) {
      MakeSpace(len);
    }

    assert(GetWritableSize() >= len);
  }

  void Shrink(size_type n);

  void swap(Buffer& other) noexcept {
    std::swap(write_index_, other.write_index_);
    std::swap(read_index_, other.read_index_);
    std::swap(data_, other.data_);
  }
  
  // set saved_errno to avoid import logger.h to Buffer.cc  
  size_type ReadFd(int fd, int& saved_errno);  
private:
  typedef data_type::const_iterator const_iterator;
  typedef data_type::iterator iterator;
  

  void MakeSpace(size_type len) {
    // if all unused space size < len + prefix size
    // only to expand buffer
    // else we can resize inside buffer through adjust layout
    // | prepend size | readable size | writable size | unused |
    //            ||
    // | prefix size | readable size | writable size | unused |
    if (len + kBufferPrefixSize >  GetPrependableSize() + GetWritableSize()) {
      // just fit
      data_.resize(write_index_ + len);
      
      // if (GetPrependableSize() < kBufferPrefixSize) {
      //   // It should be discarded
      //   auto r = read_index_ + kBufferPrefixSize - GetPrependableSize(); 
      //   std::copy(offset(r), offset(r+GetReadableSize()), 
      //       offset(kBufferPrefixSize));
      // }
    } else {
      // reuse unused space between read_index_ and writable_index
      //
      // read_index_ should after prefix size
      // i.e. response size should prepend at last
      // assert(kBufferPrefixSize <= read_index_);
      auto readable = this->GetReadableSize();

      std::copy(offset(read_index_),
            offset(write_index_),
            offset(kBufferPrefixSize));
      read_index_ = kBufferPrefixSize;
      write_index_ = read_index_ + readable;
      
      assert(readable == this->GetReadableSize());

    }

  }
  
  const_iterator offset(size_type n) const noexcept
  { return data_.cbegin() + n; }

  iterator offset(size_type n) noexcept
  { return data_.begin() + n; }

  size_type read_index_;
  size_type write_index_;
  data_type data_;
};

} // namespace kanon

#endif // KANON_NET_BUFFER_H