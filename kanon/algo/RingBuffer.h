#ifndef KANON_ALGO_RINGBUFFER_H
#define KANON_ALGO_RINGBUFFER_H

#include "kanon/util/macro.h"
#include "kanon/log/Logger.h"
#include "kanon/algo/construct.h"

#include <utility> 			// swap
#include <stdint.h>
#include <cstdlib> 			// mkstemp
#include <sys/types.h> 	// off_t
#include <sys/mman.h> 	// mmap
#include <unistd.h> 		// unlink
#include <assert.h>

namespace kanon {

// adjust size to n times page size(n is an integer)
#define ROUNDUP_PGSIZE(size) \
	(((size) + 0xfff) & (~0xfff))

// for debug
#define IS_PGSIZE(size) \
  (size & (~0xfff))

/**
 * @class RingBuffer
 * @tparam T type of element
 * @brief
 * RingBuffer write policy when buffer is full:
 * -- overwrite these old data in buffer.
 * Since memory space is linear, we want to implemete such
 * ring buffer which need use read index and write index to 
 * construct a loop logically.
 * 
 * Here, I use memory map to map two buffer to same temp file,
 * then, if read index is not begin address, the write index 
 * must after read index, but write position before read index.
 * This is different from classic implemetation that reset index
 * but memory is not continuous in fact.Memory map make memory to
 * a ring and no need to reset index(only shrink index).
 * The only one disadvantage is need double space than expected.
 */
template<typename T>	
class RingBuffer {
public:
	typedef T value_type;
	typedef size_t size_type;
	typedef T* pointer;
	typedef T const* const_pointer;
	typedef T& reference;
	typedef T const& const_reference;
	typedef pointer iterator;
	typedef const_pointer const_iterator;

	explicit RingBuffer(size_type n);
	~RingBuffer() KANON_NOEXCEPT;
  
  RingBuffer(RingBuffer const& other);
  RingBuffer(RingBuffer&& other) KANON_NOEXCEPT;

  RingBuffer& operator=(RingBuffer const& other);
  RingBuffer& operator=(RingBuffer&& other) KANON_NOEXCEPT;

	/**
	 * @return size of readable region
	 */
	size_type readable() const KANON_NOEXCEPT {
		return write_index_ - read_index_;
	}

	/**
	 * @return remaing space for writing
	 */
	size_type writeable() const KANON_NOEXCEPT {
		return count_ - readable();
	}

	/**
	 * @return bound of buffer
	 */
	size_type maxSize() const KANON_NOEXCEPT {
		return count_;
	}

	/**
	 * @return readable position 
	 */
	const_pointer peek() const KANON_NOEXCEPT {
		return data_ + read_index_;
	}
	

	pointer peek() KANON_NOEXCEPT {
		return data_ + read_index_;
	}
	
	/**
	 * @brief advance read index
	 * @warning should be called after consume readable buffer
	 */
	void advance(size_type n) KANON_NOEXCEPT {
		read_index_ += n;
		if (read_index_ > count_) {
			read_index_ -= count_;
			write_index_ -= count_;
		}
	}

	/**
	 * @brief write data to buffer 
	 * @return written number of element, 
	 */
	template<typename FI>
	size_type append(FI first, size_type n) {
		auto last = first;
		std::advance(last, n);
		return append(std::move(first), std::move(last));
	}

	template<typename FI>
	size_type append(FI first, FI last);

	/**
	 * @brief emplace element which is constructed in place
	 */
	template<typename... Args>
	void emplace(Args&&... args);

  void swap(RingBuffer& rhs) KANON_NOEXCEPT {
		std::swap(data_, rhs.data_);
		std::swap(count_, rhs.count_);
		std::swap(write_index_, rhs.write_index_);
		std::swap(read_index_, rhs.read_index_);
  }

private:
	pointer writeBegin() KANON_NOEXCEPT {
		return data_ + write_index_;
	}

	T* data_;
	size_type count_;
	size_type write_index_;
	size_type read_index_;
};

template<typename T>
RingBuffer<T>::RingBuffer(size_type n) 
	: count_{ 0 }
	, write_index_{ 0 }
	, read_index_{ 0 }
{
	// @warning: You should not use char const* to accept string literal,
	// 					 because it stored in read only memory.
	//					 However, mkstemp() will modify XXXXXX to generate unique name.
	//					 Therefore, it you do such, it will trigger segment fault.
	char const path[] = "/dev/shm/ring_buffer_XXXXXX";
	int status = 0;

	int fd = ::mkstemp(const_cast<char*>(path));
	if (fd < 0) {
		LOG_SYSFATAL << "make a temp file error";
	}

	status = ::unlink(path);	
	if (status < 0) {
		LOG_SYSERROR << "unlink temp file for ring buffer error";
	}

	count_ = ROUNDUP_PGSIZE(n * sizeof(T));
  assert(IS_PGSIZE(count_));
  
	status = ::ftruncate(fd, count_);
	if (status < 0) {
    if (close(fd) < 0) {
      LOG_SYSFATAL << "close fd error(in ftruncate error handle)";
    }
		LOG_SYSFATAL << "ftruncate temp file to count size error";
	}

	auto data = ::mmap(
		NULL, count_ << 1, 
		PROT_NONE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);

	if (data == MAP_FAILED) {
    if (close(fd) < 0) {
      LOG_SYSFATAL << "close fd error(in mmap error handle)";
    }
		LOG_SYSFATAL << "mmap error";
	}

	data_ = static_cast<pointer>(data);

	auto addr = ::mmap(
		data_, count_,
		PROT_READ | PROT_WRITE, MAP_FIXED | MAP_SHARED, fd, 0);

	if (static_cast<pointer>(addr) != data) {
    if (close(fd) < 0) {
      LOG_SYSFATAL << "close fd error(in mmap error handle)";
    }
		LOG_SYSFATAL << "mmap error";
	}

	addr = ::mmap(
  	data_ + count_, count_,
		PROT_READ | PROT_WRITE, MAP_FIXED | MAP_SHARED, fd, count_);

	if (static_cast<pointer>(addr) != data_ + count_) {
    if (close(fd) < 0) {
      LOG_SYSFATAL << "close fd error(in mmap error handle)";
    }
		LOG_SYSFATAL << "mmap error";
	}


	status = ::close(fd);

	if (status < 0) {
		LOG_SYSERROR << "close temp file for ring buffer error";
	}

  assert(read_index_ == 0);
	assert(write_index_ == 0);
  count_ = n;
}

template<typename T>
RingBuffer<T>::~RingBuffer() KANON_NOEXCEPT {
	if (data_) {
		if (::munmap(
				data_, 
				ROUNDUP_PGSIZE(count_ * sizeof(T)) << 1) < 0) {
					LOG_SYSFATAL << "munmap error";
		}

		write_index_ = read_index_ = count_ = 0;
	}
}

template<typename T>
RingBuffer<T>::RingBuffer(RingBuffer const& other) {
  RingBuffer(other.count_);

	std::copy(other.peek(), other.peek()+other.readable());
	advance(other.readable());
}

template<typename T>
RingBuffer<T>::RingBuffer(RingBuffer&& other) KANON_NOEXCEPT
{
	swap(other);
	other.data_ = NULL;
	other.count_ = other.write_index_ = other.read_index_ = 0;
}

template<typename T>
RingBuffer<T>& 
RingBuffer<T>::operator=(RingBuffer<T> const& other) {
	if (this != &other) {
		auto tmp{ other };
		swap(tmp);
		return *this;	
	}
}

template<typename T>
RingBuffer<T>&
RingBuffer<T>::operator=(RingBuffer<T>&& other) KANON_NOEXCEPT {
		// no need to free space
		swap(other);
}

template<typename T>
template<typename FI>
auto RingBuffer<T>::append(FI first, FI last) 
-> size_type {
	int remain = 0;
	const size_type writeable_size = writeable();
	const size_type n = std::distance(first, last);

	if (n > writeable_size) {
		remain = n - writeable_size;
		assert(remain < count_);
		auto mid = first;
		std::advance(mid, writeable_size);

		std::copy(std::move(first), mid, writeBegin());
		write_index_ += writeable_size;
		algo_util::destroy(peek(), peek()+remain);
		std::copy(std::move(mid), std::move(last), peek());
		write_index_ += remain;
		advance(remain);
	} else {
		remain = 0;
		std::copy(std::move(first), std::move(last), writeBegin());
		write_index_ += n;
	}

	return remain;
}

template<typename T>
template<typename... Args>
void RingBuffer<T>::emplace(Args&&... args) {
	if (writeable() >= 1) {
		algo_util::construct(writeBegin(), std::forward<Args>(args)...);
		write_index_++;
	} else {
		algo_util::destroy(peek());
		algo_util::construct(peek(), std::forward<Args>(args)...);
		write_index_++;
		advance(1);
	}
}

template<typename T>
void swap(RingBuffer<T>& lhs, RingBuffer<T>& rhs) 
#ifdef CXX_STANDARD_11
noexcept(noexcept(lhs.swap(rhs)))
#endif
{
	lhs.swap(rhs);
}

} // namespace kanon

#endif // KANON_ALGO_RINGBUFFER_H