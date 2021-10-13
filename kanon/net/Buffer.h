#ifndef KANON_NET_BUFFER_H
#define KANON_NET_BUFFER_H

#include "kanon/util/macro.h"
#include "kanon/string/string-view.h"
#include "kanon/net/endian_api.h"

#include <stdint.h>
#include <vector>

namespace kanon {

#define BUFFER_INITSIZE 1024
#define BUFFER_PREFIX_SIZE 8

/*
 * @brief simple continuous buffer
 * it provide for byte stream with operation sucha as append and prepend
 * buffer layout:
 * | prepend region | readable region | writable region |
 * * prepend region ensure 8 byte at least to let user fill packet size but no need to move space
 * * readable region is where the data is stored
 * * writable region is where data to append
 *
 * @note the buffer don't actively shrink capacity
 */ 
class Buffer {
	typedef std::vector<char> data_type;
public:
	typedef data_type::size_type size_type;
	typedef data_type::value_type value_type;
	typedef data_type::pointer pointer;
	typedef data_type::const_pointer const_pointer;

	explicit Buffer(size_type init_size = BUFFER_INITSIZE)
		: read_index_{ init_size }
		, write_index_{ init_size }
	{
		static_assert(BUFFER_PREFIX_SIZE == 8, "buffer prefix size must be 8");
		data_.resize(BUFFER_PREFIX_SIZE + init_size);
	}

	~Buffer() = default;
	
	StringView toStringView() const KANON_NOEXCEPT
	{ return { data_.data() + read_index_, static_cast<StringView::size_type>(readable_size()) }; }	

	// peek: you can understand just read but don't remove it
	// normally, this word is understanded to "secret look"
	const_pointer peek() const KANON_NOEXCEPT
	{ return data_.data() + readable_size(); }
	
	pointer peek() KANON_NOEXCEPT
	{ return data_.data() + readable_size(); }

	// append operation:
	void append(StringView str) {
		if (str.size() < writable_size())
			make_space(str.size());
		else {
			std::copy(str.begin(), str.end(), peek());
			write_index_ += str.size();
		}
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
		assert(read_index_ >= BUFFER_PREFIX_SIZE);
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
		// FIXME: may be a bug? 
		// muduo reset read_index_ = write_index_ = BUFFER_PREFIX_SIZE when n == readable_size	
		read_index_ += n;
	}	
	
	// since uint64_t in some machine which only support 32bit at most 
	// present 32bit, you can't just write advance(8)	
	void advance64() KANON_NOEXCEPT
	{ advance(sizeof(uint64_t)); }	

	void advance32() KANON_NOEXCEPT
	{ advance(sizeof(uint32_t)); }

	void advance16() KANON_NOEXCEPT
	{ advance(sizeof(uint16_t)); }
	
	// retrieve	operation:
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
		//						||
		// | prefix size | readable size | writeable size | unused |
		if (writable_size() + prependable_size() < len + BUFFER_PREFIX_SIZE) {
			// just fit
			data_.resize(write_index_ + len);
		} else {
			// reuse unused space between read_index_ and writable_index
			//
			// read_index_ should after prefix size
			// i.e. response size should prepend at last
			assert(BUFFER_PREFIX_SIZE < read_index_);
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
