#ifndef KANON_NET_BUFFER_H
#define KANON_NET_BUFFER_H

#include <stdint.h>
#include <vector>

#include "kanon/util/macro.h"
#include "kanon/string/string_view.h"

#include "kanon/net/endian_api.h"

namespace kanon {

//! \addtogroup net
//!@{

/**
 * \brief Simple continuous buffer that split three region
 *
 * It provide for byte stream with operation sucha as append and prepend.
 * Buffer layout:
 * | prepend region | readable region | writable region |
 *   * prepend region ensure 8 byte at least to let user fill packet size but no need to move space.
 *   * readable region is where the data is stored.
 *   * writable region is where data to append.
 *
 * \warning 
 *   The buffer don't actively shrink capacity.\n
 *   User should call Shrink() explicitly
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
  
  /**
   * \brief Construct a Buffer object
   * \param init_size The initialized size of write region
   */
  explicit Buffer(size_type init_size = kBufferInitSize);
  ~Buffer();
  
  //! \name conversion
  //!@{ 

  //! Convert to StringView(A readonly string slice)
  StringView ToStringView() const noexcept
  { return StringView{ data_.data() + read_index_, static_cast<StringView::size_type>(GetReadableSize()) }; }  
  
  //!@}  
  
  //! \name region pointer getter 
  //!@{
 
  //! Get the begin position of read region(Readonly)
  const_pointer GetReadBegin() const noexcept
  { return data_.data() + read_index_; }
  
  //! Get the begin position of read region
  pointer GetReadBegin() noexcept
  { return data_.data() + read_index_; }
  
  //! Get the begin position of write region
  pointer GetWriteBegin() noexcept
  { return data_.data() + write_index_; } 
  
  //!@} 
 
  //! \name search based on special delimeter
  //!@{
  
  /**
   * \brief Find the "\r\n" and put it to @p buffer if success
   * \param buffer A string view where store the contents until "\r\n"
   * \return
   *   true indicates success
   * \warning
   *   This don't advance read pointer.\n
   *   If you want retrive string from buffer, 
   *   you should use FindCrLf(string&)
   */
  bool FindCrLf(StringView& buffer) noexcept {
    auto tmp = ToStringView();
    StringView::size_type idx = 0; 

    if ((idx = tmp.find("\r\n")) == StringView::npos) {
      return false;
    } 

    buffer = tmp.substr(0, idx);
    return true;
  }

  /**
   * \brief Find the "\n" and put it to @p buffer if success
   * \param buffer A string view where store the contents until "\n"
   * \return
   *   true indicates success
   * \warning
   *   This don't advance read pointer.\n
   *   If you want retrive string from buffer, 
   *   you should use FindLf(string&)
   */
  bool FindLf(StringView& buffer) noexcept {
    auto tmp = ToStringView();
    StringView::size_type idx = 0; 

    if ((idx = tmp.find("\n")) == StringView::npos) {
      return false;
    } 

    buffer = tmp.substr(0, idx);
    return true;
  }
  
  /**
   * \brief Find the "\r\n" and put it to @p buffer if success
   * \param buffer A string where store the contents until "\r\n"
   * \return
   *   true indicates success
   */
  bool FindCrLf(std::string& str) {
    StringView tmp;
    if (FindCrLf(tmp) == true) {
      str = tmp.ToString();
      AdvanceRead(tmp.size());
      return true;
    }

    return false;
  }

  /**
   * \brief Find the "\n" and put it to @p buffer if success
   * \param buffer A string where store the contents until "\n"
   * \retur
   *   true indicates success
   */
  bool FindLf(std::string& str) {
    StringView tmp;
    if (FindLf(tmp) == true) {
      str = tmp.ToString();
      AdvanceRead(tmp.size());
      return true;
    }

    return false;
  }

  //!@}

  //! \name append operation
  //!@{

  //! Append contents of string view to buffer
  void Append(StringView str) {
    MakeSpace(str.size());
    
    std::copy(str.begin(), str.end(), offset(write_index_));
    write_index_ += str.size();
  }
  
  //! Append raw char array to buffer in @p len
  void Append(char const* str, size_t len) {
    Append(StringView(str, len));
  } 
  
 //! Append raw array to buffer in @p len 
  void Append(void const* p, size_t len) {
    Append(static_cast<char const*>(p), len);
  }  
  
  /**
   * \brief Append 64bit unsigned integer to buffer
   * \note
   *   Convert @p i to network byte order automatically
   */ 
  void Append64(uint64_t i) {
    auto ni = sock::ToNetworkByteOrder64(i);
    Append(&ni, sizeof ni);
  }  
  
  /**
   * \brief Append 32bit unsigned integer to buffer
   * \note
   *   Convert @p i to network byte order automatically
   */ 
  void Append32(uint32_t i) {
    auto ni = sock::ToNetworkByteOrder32(i);
    Append(&ni, sizeof ni);
  }

  /**
   * \brief Append 16bit unsigned integer to buffer
   * \note
   *   Convert @p i to network byte order automatically
   */ 
  void Append16(uint16_t i) {
    auto ni = sock::ToNetworkByteOrder16(i);
    Append(&ni, sizeof ni);
  }
  
  //!@} 
 
  //! \name prepend operation
  //!@{
  
  //! Prepend contents of string view to buffer
  void Prepend(StringView str) {
    Prepend(str.data(), str.size());
  }

  //! Prepend raw char array to buffer in @p len
  void Prepend(char const* str, size_t len) {
    assert(read_index_ >= kBufferPrefixSize); // First prepend
    assert(len <= read_index_);

    read_index_ -= len;
    ::memcpy(&*offset(read_index_), str, len);
  }
  
  //! Prepend raw array to buffer in @p len
  void Prepend(void const* str, size_t len) {
    Prepend(static_cast<char const*>(str), len);
  }

  /**
   * \brief Append 16bit unsigned integer to buffer
   * \note
   *   Convert @p i to network byte order automatically
   */ 
  void Prepend16(uint16_t i) {
    auto ni = sock::ToNetworkByteOrder16(i);
    Prepend(&ni, sizeof ni);  
  }

  /**
   * \brief Append 32 unsigned integer to buffer
   * \note
   *   Convert @p i to network byte order automatically
   */ 
  void Prepend32(uint32_t i) {
    auto ni = sock::ToNetworkByteOrder32(i);
    Prepend(&ni, sizeof ni);  
  }

  /**
   * \brief Append 64bit unsigned integer to buffer
   * \note
   *   Convert @p i to network byte order automatically
   */ 
  void Prepend64(uint64_t i) {
    auto ni = sock::ToNetworkByteOrder64(i);
    Prepend(&ni, sizeof ni);  
  }
  //!@}
 
  //! \name Prepend size getter
  //!@{
  
  /**
   * \brief Get the 16bit unsigned integer in the prepend region
   * \note
   *   Returned value has been converted to host byte order automatically
   */
  uint16_t GetReadBegin16() const noexcept {
    uint16_t i;
    ::memcpy(&i, GetReadBegin(), sizeof i);
      
    return sock::ToHostByteOrder16(i);
  }

  /**
   * \brief Get the 32bit unsigned integer in the prepend region
   * \note
   *   Returned value has been converted to host byte order automatically
   */
  uint32_t GetReadBegin32() const noexcept {
    uint32_t i;
    ::memcpy(&i, GetReadBegin(), sizeof i);
      
    return sock::ToHostByteOrder32(i);
  }

  /**
   * \brief Get the 64bit unsigned integer in the prepend region
   * \note
   *   Returned value has been converted to host byte order automatically
   */
  uint64_t GetReadBegin64() const noexcept {
    uint64_t i;
    ::memcpy(&i, GetReadBegin(), sizeof i);
      
    return sock::ToHostByteOrder64(i);
  }
  
  /**
   * \brief Read the 16bit unsigned integer in the prepend region
   * \note
   *   Returned value has been converted to host byte order automatically.\n
   *   Compared to GetReadBegin16(), this will advance read pointer
   */
  uint16_t Read16() noexcept {
    auto ret = GetReadBegin16();
    AdvanceRead16();
    return ret;
  }
  
  /**
   * \brief Read the 32bit unsigned integer in the prepend region
   * \note
   *   Returned value has been converted to host byte order automatically.\n
   *   Compared to GetReadBegin32(), this will advance read pointer
   */
  uint32_t Read32() noexcept {
    auto ret = GetReadBegin32();
    AdvanceRead32();
    return ret;
  }

  /**
   * \brief Read the 64bit unsigned integer in the prepend region
   * \note
   *   Returned value has been converted to host byte order automatically.\n
   *   Compared to GetReadBegin64(), this will advance read pointer
   */
  uint64_t Read64() noexcept {
    auto ret = GetReadBegin64();
    AdvanceRead64();
    return ret;
  }

  //!@}
 
  //! \name advance operation
  //!@{
  
  //! Advance the read pointer in @p n 
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

  //! Advance the read pointer to write pointer(i.e. read region become empty)
  void AdvanceAll() noexcept
  { AdvanceRead(GetReadableSize()); } 

  /**
   * \brief Advance read region in sizeof(uint64_t)
   * \note
   *   Because uint64_t just represents 32bit in some machine 
   *   that support 32bit only, you can't just write AdvanceRead(8).
   *   It is better call this in such cases.
   */  
  void AdvanceRead64() noexcept
  { AdvanceRead(sizeof(uint64_t)); }  

  /**
   * \brief Advance read region in sizeof(uint32_t)
   */
  void AdvanceRead32() noexcept
  { AdvanceRead(sizeof(uint32_t)); }

  /**
   * \brief Advance read region in sizeof(uint16_t)
   */
  void AdvanceRead16() noexcept
  { AdvanceRead(sizeof(uint16_t)); }
  
  //! Advance write pointer in @p n 
  void AdvanceWrite(size_type n) noexcept {
    assert(n <= GetWritableSize());

    write_index_ += n;
  } 
  
  //!@}
  
  //! \name retrieve operation
  //!@{
  
  //! Retrieve all readable contents as string 
  std::string RetrieveAllAsString() {
    return RetrieveAsString(GetReadableSize());
  }

  //! Retrieve readable contents as string in @p len
  std::string RetrieveAsString(size_type len) {
    assert(len <= GetReadableSize());
    std::string str(GetReadBegin(), len);
    AdvanceRead(len);
    return str;
  }
  
  /**
   * \brief Retrieve readable contents as std::vector<char> in @p len
   * \note
   *   If buffer store binary data, the std::vector<char> is more useful than
   *   std::string
   *    - std::string occupy 32 bytes but std::vector<char> just 24 bytes
   *    - std::string represents a null-terminated string in fact,
   *      std::vector<char> don't care what the contents is.
   */
  std::vector<char> RetrieveAsVector(size_type len) {
    assert(len <= GetReadableSize());

    std::vector<char> arr(GetReadBegin(), GetReadBegin() + len);
    AdvanceRead(len);

    return arr;
  }

  /**
   * \brief Retrieve all readable contents as std::vector<char>
   */
  std::vector<char> RetrieveAllAsVector() {
    return RetrieveAsVector(GetReadableSize());
  }
  
  //!@}
 
  
  //! \name metadata getter
  //!@{

  //! Get the storage data structure(In fact, this is just a std::vector<char>)
  data_type const& GetData() const noexcept
  { return data_; }
  
  //! Get the size of prepend region
  size_type GetPrependableSize() const noexcept
  { return read_index_; }
  
  //! Ask whether has readable contents
  bool HasReadable() const noexcept
  { return read_index_ != write_index_; }
  
  //! Get the size of read region
  size_type GetReadableSize() const noexcept
  { return write_index_ - read_index_; }
  
  //! Get the sizeof write region 
  size_type GetWritableSize() const noexcept
  { return data_.size() - write_index_; }
  
  //! Get the capacity of storage data structure 
  size_type GetCapacity() const noexcept
  { return data_.capacity(); }
  
  //!@} 
  
  //! \name capacity opertion
  //!@{
 
  //! Like std::vector::reserve(), to ensure buffer has @p len space at least
  void ReserveWriteSpace(uint32_t len) noexcept {
    if (len > GetWritableSize()) {
      MakeSpace(len);
    }

    assert(GetWritableSize() >= len);
  }
  
  //! Like std::vector::shrink_to_fit(), to shrink the buffer to (readable size + 8)
  void Shrink(size_type n = 0);
  
  //!@}
 
  void swap(Buffer& other) noexcept {
    std::swap(write_index_, other.write_index_);
    std::swap(read_index_, other.read_index_);
    std::swap(data_, other.data_);
  }
  
  /**
   * \brief Read contents from @p fd and put them to buffer
   * \param fd File descriptor that must be a socket
   * \param saved_errno Save the errno that returned by ::read()
   *
   * \note 
   *   This is a internal API, user don't care this.\n
   *   Then saved_errno is necessary since we don't process the error here.
   */
  size_type ReadFd(int fd, int& saved_errno);  
private:
  typedef data_type::const_iterator const_iterator;
  typedef data_type::iterator iterator;
  
  void MakeSpace(size_type len);
  
  const_iterator offset(size_type n) const noexcept
  { return data_.cbegin() + n; }

  iterator offset(size_type n) noexcept
  { return data_.begin() + n; }

  size_type read_index_; //!< Read pointer(i.e. begin position of read region)
  size_type write_index_; //!< Write pointer(i.e. begin position of write region)
  data_type data_; //!< Store the contents
};

//!@}
} // namespace kanon

#endif // KANON_NET_BUFFER_H
