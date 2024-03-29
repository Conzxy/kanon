#ifndef KANON_BUFFER_BUFFER_H
#define KANON_BUFFER_BUFFER_H

#include <stdint.h>

#include "kanon/util/macro.h"
#include "kanon/string/string_view.h"

#include "kanon/util/endian_api.h"
#include "kanon/algo/reserved_array.h"

namespace kanon {

//! \addtogroup buffer
//!@{

/**
 * \brief Simple continuous buffer that split three region
 *
 * It provide for byte stream with operation such as append and prepend.
 * Buffer layout:
 * | prepend region | readable region | writable region |
 *   * prepend region ensure 8 byte at least to let user fill packet size but no
 * need to move space.
 *   * readable region is where the data is stored.
 *   * writable region is where data to append.
 *
 * \warning
 *   The buffer don't actively shrink capacity.\n
 *   User should call Shrink() explicitly
 */
class Buffer {
  using data_type = algo::ReservedArray<char>;

  static constexpr int BUFFER_INIT_SIZE = 1024;
  static constexpr int BUFFER_PREFIX_SIZE = 8;

 public:
  using size_type = data_type::size_type;
  using value_type = data_type::value_type;
  using pointer = data_type::pointer;
  using const_pointer = data_type::const_pointer;

  /**
   * \brief Construct a Buffer object
   * \param init_size The initialized size of write region
   */
  KANON_CORE_API explicit Buffer(size_type init_size = BUFFER_INIT_SIZE);
  KANON_CORE_API ~Buffer();

  //! \name conversion
  //!@{

  //! Convert to StringView(A readonly string slice)
  StringView ToStringView() const KANON_NOEXCEPT
  {
    return StringView{data_.data() + read_index_,
                      static_cast<StringView::size_type>(GetReadableSize())};
  }

  //!@}

  //! \name region pointer getter
  //!@{

  //! Get the begin position of read region(Readonly)
  const_pointer GetReadBegin() const KANON_NOEXCEPT
  {
    return data_.data() + read_index_;
  }

  //! Get the begin position of read region
  pointer GetReadBegin() KANON_NOEXCEPT { return data_.data() + read_index_; }

  //! Get the begin position of write region
  pointer GetWriteBegin() KANON_NOEXCEPT { return data_.data() + write_index_; }

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
  bool FindCrLf(StringView &buffer) KANON_NOEXCEPT
  {
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
  bool FindLf(StringView &buffer) KANON_NOEXCEPT
  {
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
  bool FindCrLf(std::string &str)
  {
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
  bool FindLf(std::string &str)
  {
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
  void Append(StringView str)
  {
    ReserveWriteSpace(str.size());

    std::copy(str.begin(), str.end(), offset(write_index_));
    write_index_ += str.size();
  }

  //! Append raw char array to buffer in @p len
  void Append(char const *str, size_t len) { Append(StringView(str, len)); }

  //! Append raw array to buffer in @p len
  void Append(void const *p, size_t len)
  {
    Append(static_cast<char const *>(p), len);
  }

#define Append_Macro(size)                                                     \
  void Append##size(uint##size##_t i)                                          \
  {                                                                            \
    auto ni = sock::ToNetworkByteOrder##size(i);                               \
    Append(&ni, sizeof ni);                                                    \
  }

  /**
   * \brief Append 64bit unsigned integer to buffer
   * \note
   *   Convert @p i to network byte order automatically
   */
  Append_Macro(64)

  /**
   * \brief Append 32bit unsigned integer to buffer
   * \note
   *   Convert @p i to network byte order automatically
   */
  Append_Macro(32)

  /**
   * \brief Append 16bit unsigned integer to buffer
   * \note
   *   Convert @p i to network byte order automatically
   */
  Append_Macro(16)

  void Append8(uint8_t i) { Append(&i, sizeof i); }

  //!@}

  //! \name prepend operation
  //!@{

  //! Prepend contents of string view to buffer
  void Prepend(StringView str) { Prepend(str.data(), str.size()); }

  //! Prepend raw char array to buffer in @p len
  void Prepend(char const *str, size_t len)
  {
    assert(read_index_ >= BUFFER_PREFIX_SIZE); // First prepend
    assert(len <= read_index_);

    read_index_ -= len;
    ::memcpy(&*offset(read_index_), str, len);
  }

  //! Prepend raw array to buffer in @p len
  void Prepend(void const *str, size_t len)
  {
    Prepend(static_cast<char const *>(str), len);
  }

  /**
   * \brief Prepend 8bit unsigned integer to buffer
   * \note
   *   No need to convert @p i to network byte order
   */
  void Prepend8(uint8_t i) { Prepend(&i, sizeof i); }

#define Prepend_Macro(size)                                                    \
  void Prepend##size(uint##size##_t i)                                         \
  {                                                                            \
    auto ni = sock::ToNetworkByteOrder##size(i);                               \
    Prepend(&ni, sizeof ni);                                                   \
  }

  /**
   * \brief Prepend 16bit unsigned integer to buffer
   * \note
   *   Convert @p i to network byte order automatically
   */
  Prepend_Macro(16)

  /**
   * \brief Prepend 32 unsigned integer to buffer
   * \note
   *   Convert @p i to network byte order automatically
   */
  Prepend_Macro(32)

  /**
   * \brief Prepend 64bit unsigned integer to buffer
   * \note
   *   Convert @p i to network byte order automatically
   */
  Prepend_Macro(64)

  //!@}

  //! \name Prepend size getter
  //!@{

#define GetReadBegin_Macro(size)                                               \
  uint##size##_t GetReadBegin##size() const KANON_NOEXCEPT                     \
  {                                                                            \
    uint##size##_t i;                                                          \
    ::memcpy(&i, GetReadBegin(), sizeof i);                                    \
    return sock::ToHostByteOrder##size(i);                                     \
  }

  /**
   * \brief Get the 8bit unsigned integer in the prepend region
   * \note
   *   Returned value no need to be converted to host byte order
   */
  uint8_t GetReadBegin8() const KANON_NOEXCEPT
  {
    uint8_t i;
    ::memcpy(&i, GetReadBegin(), sizeof i);

    return i;
  }

  /**
   * \brief Get the 16bit unsigned integer in the prepend region
   * \note
   *   Returned value has been converted to host byte order automatically
   */
  GetReadBegin_Macro(16)

  /**
   * \brief Get the 32bit unsigned integer in the prepend region
   * \note
   *   Returned value has been converted to host byte order automatically
   */
  GetReadBegin_Macro(32)

  /**
   * \brief Get the 64bit unsigned integer in the prepend region
   * \note
   *   Returned value has been converted to host byte order automatically
   */
  GetReadBegin_Macro(64)

#define Read_Macro(size)                                                       \
  uint##size##_t Read##size() KANON_NOEXCEPT                                   \
  {                                                                            \
    auto ret = GetReadBegin##size();                                           \
    AdvanceRead##size();                                                       \
    return ret;                                                                \
  }

  /**
   * \brief Read the 8bit unsigned integer in the prepend region
   * \note
   *   Compared to GetReadBegin8(), this will advance read pointer
   */
  Read_Macro(8)

  /**
   * \brief Read the 16bit unsigned integer in the prepend region
   * \note
   *   Returned value has been converted to host byte order automatically.\n
   *   Compared to GetReadBegin16(), this will advance read pointer
   */
  Read_Macro(16)

  /**
   * \brief Read the 32bit unsigned integer in the prepend region
   * \note
   *   Returned value has been converted to host byte order automatically.\n
   *   Compared to GetReadBegin32(), this will advance read pointer
   */
  Read_Macro(32)

  /**
   * \brief Read the 64bit unsigned integer in the prepend region
   * \note
   *   Returned value has been converted to host byte order automatically.\n
   *   Compared to GetReadBegin64(), this will advance read pointer
   */
  Read_Macro(64)

  //!@}

  //! \name advance operation
  //!@{

  //! Advance the read pointer in @p n
  void AdvanceRead(size_type n) KANON_NOEXCEPT
  {
    assert(n <= GetReadableSize());

    // Reset read and write index to
    // make GetWritableSize() larger,
    // then reduce the number of the call to ReserveWriteSpace()
    // (Although it also reset index in ReserveWriteSpace()
    // but now is a good chance to reset).
    if (n == GetReadableSize()) {
      read_index_ = write_index_ = BUFFER_PREFIX_SIZE;
    } else {
      read_index_ += n;
    }
  }

  //! Advance the read pointer to write pointer(i.e. read region become empty)
  void AdvanceAll() KANON_NOEXCEPT { AdvanceRead(GetReadableSize()); }

#define AdvanceRead_Macro(size)                                                \
  void AdvanceRead##size() KANON_NOEXCEPT                                      \
  {                                                                            \
    AdvanceRead(sizeof(uint##size##_t));                                       \
  }

  /**
   * \brief Advance read region in sizeof(uint64_t)
   * \note
   *   Because uint64_t just represents 32bit in some machine
   *   that support 32bit only, you can't just write AdvanceRead(8).
   *   It is better call this in such cases.
   */
  AdvanceRead_Macro(64)

  /**
   * \brief Advance read region in sizeof(uint32_t)
   */
  AdvanceRead_Macro(32)

  /**
   * \brief Advance read region in sizeof(uint16_t)
   */
  AdvanceRead_Macro(16)

  /**
   * \brief Advance read region in sizeof(uint8_t)
   */
  AdvanceRead_Macro(8)

  //! Advance write pointer in @p n
  void AdvanceWrite(size_type n) KANON_NOEXCEPT
  {
    assert(n <= GetWritableSize());

    write_index_ += n;
  }

  void SetWriteIndex(size_type n) KANON_NOEXCEPT
  {
    assert(n >= read_index_ && n <= GetCapacity());
    write_index_ = n;
  }
  //!@}

  //! \name retrieve operation
  //!@{

  //! Retrieve all readable contents as string
  std::string RetrieveAllAsString()
  {
    return RetrieveAsString(GetReadableSize());
  }

  //! Retrieve readable contents as string in @p len
  std::string RetrieveAsString(size_type len)
  {
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
  std::vector<char> RetrieveAsVector(size_type len)
  {
    assert(len <= GetReadableSize());

    std::vector<char> arr(GetReadBegin(), GetReadBegin() + len);
    AdvanceRead(len);

    return arr;
  }

  /**
   * \brief Retrieve all readable contents as std::vector<char>
   */
  std::vector<char> RetrieveAllAsVector()
  {
    return RetrieveAsVector(GetReadableSize());
  }

  //!@}

  //! \name metadata getter
  //!@{

  //! Get the storage data structure(In fact, this is just a std::vector<char>)
  data_type const &GetData() const KANON_NOEXCEPT { return data_; }

  //! Get the size of prepend region
  size_type GetPrependableSize() const KANON_NOEXCEPT { return read_index_; }

  //! Ask whether has readable contents
  bool HasReadable() const KANON_NOEXCEPT
  {
    return read_index_ != write_index_;
  }

  //! Get the size of read region
  size_type GetReadableSize() const KANON_NOEXCEPT
  {
    return write_index_ - read_index_;
  }

  //! Get the sizeof write region
  size_type GetWritableSize() const KANON_NOEXCEPT
  {
    return data_.size() - write_index_;
  }

  //! Get the capacity of storage data structure
  size_type GetCapacity() const KANON_NOEXCEPT
  // { return data_.capacity(); }
  {
    return data_.size();
  }

  //!@}

  //! \name capacity opertion
  //!@{

  //! Like std::vector::reserve(), to ensure buffer has @p len space at least
  KANON_CORE_API void ReserveWriteSpace(size_type len) KANON_NOEXCEPT;

  //! Like std::vector::shrink_to_fit(), to shrink the buffer to (readable size
  //! + 8)
  KANON_CORE_API void Shrink(size_type n = 0);

  //!@}

  void swap(Buffer &other) KANON_NOEXCEPT
  {
    std::swap(write_index_, other.write_index_);
    std::swap(read_index_, other.read_index_);
    std::swap(data_, other.data_);
  }

 private:
  typedef data_type::const_iterator const_iterator;
  typedef data_type::iterator iterator;

  const_iterator offset(size_type n) const KANON_NOEXCEPT
  {
    return data_.cbegin() + n;
  }

  iterator offset(size_type n) KANON_NOEXCEPT { return data_.begin() + n; }

  size_type read_index_; //!< Read pointer(i.e. begin position of read region)
  size_type
      write_index_; //!< Write pointer(i.e. begin position of write region)
  data_type data_;  //!< Store the contents
};

//!@}

} // namespace kanon

#endif // KANON_BUFFER_BUFFER_H
