#include "kanon/net/Buffer.h"
#include <sys/uio.h>

using namespace kanon;

Buffer::size_type
Buffer::readFd(int fd, int& saved_errno) {
	char extra_buf[65536]; // 64k

	struct iovec vec[2];
	vec[0].iov_base = &*offset(write_index_);
	vec[0].iov_len = writable_size();

	vec[1].iov_base = extra_buf;
	vec[1].iov_len = sizeof extra_buf;
	
	// if writable_size() less than extra_buf, we use two block, then
	// we can read 128k-1 at most.
	// otherwise, we can read writable_size() at most
	// In fact, I think we shouldn't need so big block to read data.
	size_t vec_count = writable_size() < sizeof extra_buf ? 2 : 1;

	auto readen_bytes = ::readv(fd, vec, vec_count);
	auto cache_writable_size = writable_size();

	if (readen_bytes < 0) {
		saved_errno = errno;
	} else if (static_cast<size_type>(readen_bytes) <= writable_size()) {
		write_index_ += readen_bytes;	
	} else {
		write_index_ = data_.size();
		append(extra_buf, readen_bytes - cache_writable_size);
	}

	return readen_bytes;
}

void
Buffer::shrink(size_type n) {
#if defined(CXX_STANDARD_11) && defined(SELF_DEFINED_SHRINK)
	data_.shrink_to_fit();
#else
	Buffer tmp;
	tmp.make_space(readable_size() + n);
	tmp.append(toStringView());
	swap(tmp);
#endif
}
