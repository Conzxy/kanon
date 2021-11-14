#ifndef KANON_THREAD_H
#define KANON_THREAD_H

#include "kanon/util/noncopyable.h"
#include "kanon/util/macro.h"
#include "Atomic.h"
#include "current_thread.h"

#include <pthread.h>
#include <functional>
#include <utility>

namespace kanon{

// Since std::thread use "type erase" to erase these type
// that are function type and the parameter types.
// The technique use the base class and (pure) virtual member function
// Is is need some cost to call true function dynamically.
// @see https://www.zhihu.com/question/30553807
// 
// In fact, use void() and lambda capture list, you can wrapper any function
// regardless of return type and arguments.
//
// If you want support any function signature, you can use std::make_index_list<>(c++14 maybe)
// or you write a same by yourself, and combine std::tuple<> and perfect forward to complete
// pack and unpack arguments, then call corresponding function you want.
class Thread : noncopyable {
public:
  // Callback register:
  // use the std::function<> to accept any callables
  // including functions(or function pointers), function object,
  // and lambda object.
  using Threadfunc = std::function<void()>;
public:
  explicit Thread(Threadfunc func, std::string const& name = {});
  ~Thread();

  // move constructor
  // so that thread can be placed in container by move construct
  // or use pointer(OR better?)
  Thread(Thread&& rhs) KANON_NOEXCEPT
    : func_{std::move(rhs.func_)},
      is_started_{rhs.is_started_},
      is_joined_{rhs.is_joined_},
      pthreadId_{rhs.pthreadId_},
      name_{std::move(rhs.name_)}
  {
    rhs.pthreadId_ = 0;
  }
  
  Thread& operator=(Thread&& rhs) KANON_NOEXCEPT {
    func_ = std::move(rhs.func_);
    is_started_ = rhs.is_started_;
    is_joined_ = rhs.is_joined_;
    pthreadId_ = rhs.pthreadId_;
    name_ = std::move(rhs.name_);

    rhs.pthreadId_ = 0;
    rhs.is_started_ = false;
    
    return *this;
  }

  void start();
  void join();

  std::string name() const KANON_NOEXCEPT
  { return name_; }

  pthread_t pthreadId() const KANON_NOEXCEPT
  { return pthreadId_; }

private:
  void setDefaultname();

private:
  Threadfunc  func_;
  bool    is_started_;
  bool    is_joined_;
  pthread_t  pthreadId_;
  std::string name_;

  static AtomicInt32 numCreated_;
};

}//namespace kanon

#endif //_THREAD_H