#ifndef KANON_STRING_VIEW_H
#define KANON_STRING_VIEW_H

#include "kanon/util/macro.h"

#include <stdexcept>
#include <string>
#include <string.h>
#include <utility>
#include <vector>
#include <assert.h>

namespace kanon{

class StringArg
{
public:
	StringArg(char const* str)
		: data_(str)
	{ }
	
	StringArg(std::string const& str)
		: data_(str.c_str())
	{ }
	
	constexpr char const* data() const KANON_NOEXCEPT { return data_; }

	operator char const* () const
	{ return data_; }

private:
	char const* data_;
};

// only support std::string
// because I just use it
//
// StringView just view which is read only
// so the life time of string(or character) depend on user
//
class StringView
{
public:	
	using size_type = unsigned;
	using value_type = char;
	using pointer = char*;
	using const_pointer = char const*;
	using reference = char&;
	using const_reference = char const&;
	using iterator = pointer;
	using const_iterator = const_pointer;
	
	// constructor
	StringView() = default;

	template<uint64_t N>
	StringView(char const(&str)[N])
		: data_(static_cast<char const*>(str))
		, len_(sizeof str)
	{ }

	StringView(char const* str)
		: data_(str), len_(strlen(str))
	{ }
	
	constexpr StringView(char const* str, size_type len)
		: data_(str), len_(len) 
	{ }

	StringView(std::string const& str)
		: data_(str.c_str()), len_(str.size())
	{ }
	
	StringView(unsigned char const* str)
		: data_(reinterpret_cast<char const*>(str)), len_(strlen(data_))
	{ }

	StringView(unsigned char const* str, size_type len)
		: data_(reinterpret_cast<char const*>(str)), len_(len)
	{ }
	
	// wrong for char
	// because it may be prvalue
	
	// convert 
	constexpr operator char const*() const { return data_; }

	// capacity
	constexpr bool empty() const KANON_NOEXCEPT
	{ return len_ == 0; }

	constexpr size_type size() const KANON_NOEXCEPT
	{ return len_; }
	
	// position
	constexpr char const* begin() const KANON_NOEXCEPT
	{ return data_; }

	constexpr char const* end() const KANON_NOEXCEPT
	{ return data_ + len_; }
	
	 // data access
	constexpr const_reference operator[](size_type n) const KANON_NOEXCEPT
	{ return data_[n]; }

	constexpr const_reference at(size_type n) const 
	{ 
		return n >= len_ ? 
			throw std::length_error{"Given index over or equal length of StringView"} :
			data_[n];
	}
	
	constexpr const_reference front() const KANON_NOEXCEPT
	{ return data_[0]; }

	constexpr const_reference back() const KANON_NOEXCEPT
	{ return data_[len_ - 1]; }
	
	constexpr const_pointer data() const KANON_NOEXCEPT
	{ return data_; }
	
	void swap(StringView& rhs) KANON_NOEXCEPT
	{
		std::swap(data_, rhs.data_);
		std::swap(len_, rhs.len_);
	}
	
	// modify operation
	KANON_CONSTEXPR void remove_prefix(size_type n) KANON_NOEXCEPT
	{
		data_ += n;
		len_ -= n;
	}

	KANON_CONSTEXPR void remove_suffix(size_type n) KANON_NOEXCEPT
	{ len_ -= n; }

	KANON_CONSTEXPR size_type copy(char* dst, 
			size_type count, 
			size_type pos = 0) const
	{
		if(pos > len_)
			throw std::out_of_range{"Given position over length of StringView"};
		memcpy(dst, data_ + pos, count);
		return count;
	}

	KANON_CONSTEXPR StringView substr(size_type pos = 0,
			size_type count = npos) const KANON_NOEXCEPT
	{
		auto len = std::min(count, len_ - pos);
		return StringView(data_ + pos, len);
	}
	
	size_type find(StringView v, size_type pos = 0) const KANON_NOEXCEPT
	{
		// FIXME intead of kmp or other efficient algorithm
		char const* begin;
		if(v.size() > len_ || 
				!(begin = strstr(data_ + pos, v.data())))
			return npos;
		else
			return begin - data_;
	}
	
	size_type find(char c, size_type pos = 0) const KANON_NOEXCEPT
	{
		for (unsigned i = pos; i < len_; ++i)
		{
			if (c == data_[i])
				return i;
		}

		return npos;
	}

	size_type rfind(StringView v, size_type pos = npos) const KANON_NOEXCEPT
	{
		int len = std::min(len_ - 1, pos);	
		for(;len >= 0 ; --len){
			if(data_[len] == v.front() && len_ - len >= v.size() &&
					memcmp(data_ + len, v.data(), v.size()) == 0)
				return len;
		}

		return npos;
	}

	size_type rfind(char c, size_type pos = npos) const KANON_NOEXCEPT
	{
		int len = std::min(len_ - 1, pos);
		for (; len >= 0; --len)
		{
			if (data_[len] == c)
				return len;
		}

		return npos;
	}
	
	bool contains(StringView v) const KANON_NOEXCEPT
	{ return find(v) != npos; }
	
	bool contains(char c) const KANON_NOEXCEPT
	{ return find(c) != npos; }
		
	bool starts_with(StringView v) const KANON_NOEXCEPT
	{
		return (len_ >= v.size() && memcmp(data_, v.data(), v.size()) == 0) ? true : false;
	}
	
	bool starts_with(char c) const KANON_NOEXCEPT
	{
		return (len_ >= 1 && data_[0] == c) ? true : false;
	}

	bool ends_with(StringView v) const KANON_NOEXCEPT
	{
		return (len_ >= v.size() && memcmp(data_ + len_ - v.size(), v.data(), v.size()) == 0) ?
			true : false;
	}
	
	bool ends_with(char c) const KANON_NOEXCEPT
	{
		return (len_ >= 1 && data_[len_ - 1] == c) ? true : false;
	}

	/**
	 * @brief finds the first character equal to one of characters in the given character sequence
	 * @return the index of the first occurrence of any character of the sequence, if not found, return npos
	 */
	size_type find_first_of(StringView v, size_type pos = 0) const KANON_NOEXCEPT
	{ 
		for(; pos < len_; ++pos){
			if(charInRange(data_[pos], v))
				return pos;
		}

		return npos;
	}
	
	size_type find_first_of(char c, size_type pos = 0) const KANON_NOEXCEPT
	{
		for (; pos < len_; ++pos)
			if (data_[pos] == c)
				return pos;

		return npos;
	}

	/**
	 * @brief finds the last character equal to one of characters in the given character sequence(i.e. character range)
	 * @param pos the end position of the search range
	 * @return the index of the last occurrence of any character of the sequence, if not found, return npos
	 */
	size_type find_last_of(StringView v, size_type pos = npos) const KANON_NOEXCEPT
	{ 
		int i = static_cast<int>(std::min(len_ - 1, pos));

		for(; i >= 0; --i){
			if(charInRange(data_[i], v))
				return i;
		}

		return npos;
	}
	
	size_type find_last_of(char c, size_type pos = npos) const KANON_NOEXCEPT
	{
		int i = std::min(len_ - 1, pos);

		for (; i>=0; --i)
			if (data_[i] == c)
				return pos;

		return npos;
	}

	size_type find_first_not_of(StringView v, size_type pos = 0)
	{
		for(; pos < len_; ++pos){
			if(!charInRange(data_[pos], v))
				return pos;
		}
		return npos;
	}
	
	size_type find_first_not_of(char c, size_type pos = 0)
	{
		for(; pos < len_; ++pos){
			if(data_[pos] == c)
				return pos;
		}
		return npos;
	}

	size_type find_last_not_of(StringView v, size_type pos = npos)
	{
		int i = static_cast<int>(std::min(len_ - 1, pos));

		for(; i >= 0; --i){
			if(!charInRange(data_[i], v))
				return i;
		}

		return npos;
	}

	size_type find_last_not_of(char c, size_type pos = npos)
	{
		int i = static_cast<int>(std::min(len_ - 1, pos));

		for(; i >= 0; --i){
			if(data_[i] == c)
				return i;
		}

		return npos;
	}
	
	// lexicographic compare
	int compare(StringView v) const KANON_NOEXCEPT
	{
		int r = memcmp(data_, v.data(), len_ < v.size() ?
				len_ : v.size());

		if(r == 0){
			if(len_ < v.size()) r = -1;
			else r = 1;
		}

		return r;
	}

	int compare(char c) const KANON_NOEXCEPT
	{
		if (len_ < 1 || data_[0] < c)
			return -1;
		
		if (data_[0] > c)
			return 1;

		return (data_[0] == c && len_ == 1) ? 0 : 1;
	}
	
	std::vector<std::string> split(StringView spliter = " ") const KANON_NOEXCEPT {
		size_type indicator_begin = 0;
		size_type indicator_end = find(spliter);
		std::vector<std::string> ret{};
		std::string tmp;		

		while (true) {
			if (indicator_end == npos)
				indicator_end = len_;
			auto str = substr(indicator_begin, indicator_end - indicator_begin);
			tmp.assign(str.data_, str.len_);
			ret.emplace_back(std::move(tmp));

			if (indicator_end == len_) break;
			indicator_begin = indicator_end + spliter.size();
			indicator_end = find(spliter, indicator_begin);
		}
		
		return ret;
	}

private:
	// helper
	// complexity O(sv.size())
	bool charInRange(char c, StringView const& sv) const KANON_NOEXCEPT
	{
		for(auto x : sv)
			if(c == x) return true;
	
		return false;
	}

private:
	char const* data_;
	size_type len_;

	// static
	// as end indicator or error indicator
public:
	static constexpr size_type npos = -1;
};

inline void swap(StringView& lhs, StringView& rhs) KANON_NOEXCEPT(KANON_NOEXCEPT(lhs.swap(rhs)))
{ lhs.swap(rhs); }

inline bool operator==(StringView const& lhs, StringView const& rhs) KANON_NOEXCEPT
{ return (lhs.size() == rhs.size()) && memcmp(lhs.data(), rhs.data(), lhs.size()) == 0; }

inline bool operator!=(StringView const& lhs, StringView const& rhs) KANON_NOEXCEPT
{ return !(lhs == rhs); }

inline bool operator<(StringView const& lhs, StringView const& rhs) KANON_NOEXCEPT
{ return lhs.compare(rhs) < 0; }

inline bool operator>(StringView const& lhs, StringView const& rhs) KANON_NOEXCEPT
{ return rhs < lhs; }

inline bool operator>=(StringView const& lhs, StringView const& rhs) KANON_NOEXCEPT
{ return !(lhs < rhs); }

inline bool operator<=(StringView const& lhs, StringView const& rhs) KANON_NOEXCEPT
{ return !(lhs > rhs); }

template<typename Ostream>
Ostream& operator<<(Ostream& os, StringView const& v) KANON_NOEXCEPT
{ return os << v.data(); }

namespace literal{
	constexpr StringView operator""_sv(char const* str, std::size_t len) KANON_NOEXCEPT
	{ return StringView(str, len); }

} // namespace literal

} // namespace kanon

#endif // KANON_STRING_VIEW_H
