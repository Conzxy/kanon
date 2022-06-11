#ifndef KANON_ALGO_FIXED_VECTOR_H
#define KANON_ALGO_FIXED_VECTOR_H

#include <stdexcept>
#include <assert.h>

#include "kanon/util/macro.h"

#include "construct.h"

namespace kanon {

template<typename T>
class FixedVector {
public:
  explicit FixedVector(size_t size)
  : data_(nullptr)
  , size_(size)
  {
    data_ = static_cast<T*>(::malloc(sizeof(T)*size));

    if (!data_) {
      throw std::bad_alloc();
    }
  }

  ~FixedVector() noexcept {
    algo_util::destroy(data_, data_+size_);
    ::free(data_);
  }

  template<typename... Args>
  void Construct(size_t idx, Args&&... args) {
    assert(idx < size_);
    try {
      algo_util::construct(&data_[idx], std::forward<Args>(args)...);
    } catch (std::exception const& ex) {
      ::fprintf(stderr, "Caught std::exception: %s\n", ex.what());
      KANON_RETHROW;
    } catch (...) {
      ::fprintf(stderr, "Caught unknow exception");
      KANON_RETHROW;
    }
  }

  T* begin() noexcept { return data_; }
  T* end() noexcept { return data_ + size_; }
  T const* begin() const noexcept { return data_; }
  T const* end() const noexcept { return data_ + size_; }
  T const* cbegin() const noexcept { return data_; }
  T const* cend() const noexcept { return data_ + size_; }
  T const* cbegin() noexcept { return data_; }
  T const* cend() noexcept { return data_ + size_; }

  T const* data() const noexcept { return data_; }
  T* data() noexcept { return data_; }

  size_t size() const noexcept { return size_; }

  T& operator[](size_t idx) noexcept {
    return data_[idx];
  }

  T const& operator[](size_t idx) const noexcept {
    return data_[idx];
  }
private:
  T* data_;
  size_t size_;
};

} // namespace kanon

#endif // KANON_ALGO_FIXED_VECTOR_H