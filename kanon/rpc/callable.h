#ifndef KANON_RPC_CALLABLE_H__
#define KANON_RPC_CALLABLE_H__

#include <functional>

#include <google/protobuf/stubs/callback.h>

namespace kanon {

namespace protobuf {
namespace rpc {

using ::google::protobuf::Closure;

namespace internal {

class Callable : public ::google::protobuf::Closure {
  using callable_t = std::function<void()>;

 public:
  Callable(callable_t c, bool d)
    : callable_(std::move(c))
    , self_delete_(d)
  {
  }

  virtual ~Callable() override = default;

  void Run() override
  {
    callable_();
    if (self_delete_) delete this;
  }

 private:
  callable_t callable_;
  bool self_delete_;
};

} // namespace internal

inline Closure *NewCallable(std::function<void()> callable)
{
  return new internal::Callable(std::move(callable), true);
}

inline Closure *NewPermanentCallable(std::function<void()> callable)
{
  return new internal::Callable(std::move(callable), false);
}

} // namespace rpc
} // namespace protobuf
} // namespace kanon

#endif