#ifndef KANON_THREAD_H
#define KANON_THREAD_H

#include "kanon/util/macro.h"

#ifdef KANON_ON_UNIX
#include <pthread.h>
#else
#include <thread>
#endif

#include <functional>
#include <utility>
#include <string>

#include "kanon/util/noncopyable.h"
#include "kanon/thread/atomic_counter.h"
#include "kanon/thread/current_thread.h"

namespace kanon {

namespace detail {
KANON_CORE_NO_API void *Run(void *arg);
}

// Since std::thread use "type erase" to erase these type
// that are function type and the parameter types.
// The technique use the base class and (pure) virtual member function
// Is is need some cost to call true function dynamically.
// \see https://www.zhihu.com/question/30553807
//
// In fact, use std::function<void()> and lambda capture list,
// you can wrapper any function, regardless of return type and arguments.
//
// If you want support any function signature, you can use
// std::make_index_list<>(c++14 maybe) or you write a same by yourself, and
// combine std::tuple<> and perfect forward to complete pack and unpack
// arguments, then call corresponding function you want.
class Thread : noncopyable {
 public:
  // Callback register:
  // use the std::function<> to accept any callables
  // including functions(or function pointers), function object,
  // and lambda object.
  using Threadfunc = std::function<void()>;
  using ThreadId = uint64_t;

 public:
  KANON_CORE_API Thread(std::string const &name = {});

  KANON_CORE_API explicit Thread(Threadfunc func, std::string const &name = {});
  KANON_CORE_API ~Thread();

  // move constructor
  // so that thread can be placed in container by move construct
  // or use pointer(OR better?)
  Thread(Thread &&rhs) KANON_NOEXCEPT
    : func_{std::move(rhs.func_)}
    , is_started_{rhs.is_started_}
    , is_joined_{rhs.is_joined_}
    , pthreadId_{rhs.pthreadId_}
    , name_{std::move(rhs.name_)}
  {
    rhs.pthreadId_ = 0;
  }

  Thread &operator=(Thread &&rhs) KANON_NOEXCEPT
  {
    func_ = std::move(rhs.func_);
    is_started_ = rhs.is_started_;
    is_joined_ = rhs.is_joined_;
    pthreadId_ = rhs.pthreadId_;
    name_ = std::move(rhs.name_);

    rhs.pthreadId_ = 0;
    rhs.is_started_ = false;

    return *this;
  }

  KANON_CORE_API void StartRun();
  KANON_CORE_API void StartRun(Threadfunc cb);
  KANON_CORE_API void Join();

  std::string tidName() const KANON_NOEXCEPT { return name_; }

  ThreadId GetPthreadId() const KANON_NOEXCEPT { return pthreadId_; }

 private:
  KANON_CORE_NO_API void SetDefaultName();

  friend void *detail::Run(void *arg);

 private:
  Threadfunc func_;
  bool is_started_;
  bool is_joined_;
  ThreadId pthreadId_;
  std::string name_;

#if !defined(KANON_ON_UNIX)
  std::thread thr_;
#endif
  KANON_CORE_NO_API static AtomicCounter32 numCreated_;
};

} // namespace kanon

#endif //_THREAD_H
