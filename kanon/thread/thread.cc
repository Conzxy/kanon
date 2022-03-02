#include "kanon/thread/thread.h"
#include "kanon/thread/atomic.h"
#include "kanon/thread/current_thread.h"

#include "kanon/util/noncopyable.h"
#include "kanon/util/macro.h"
#include "kanon/util/unique_ptr.h"

#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <sys/prctl.h>

using namespace std;

namespace kanon{

namespace detail{

// thread entry function(i.e. void*(void*)) wrapper
// class ThreadData : noncopyable
// {
// public:
//   using Threadfunc = Thread::Threadfunc;

//   ThreadData(Threadfunc func, string const& name);
//   ~ThreadData() = default;

//   void run() const;
// private:
//   Threadfunc func_;
//   string     name_;
// };

// ThreadData::ThreadData(Threadfunc func, string const& name)
//   : func_{std::move(func)}, 
//     name_{name}
// {}

// void ThreadData::run() const{
//   KANON_UNUSED(CurrentThread::tid());
//   CurrentThread::t_name = name_.empty() ? "KanonThread" : name_.c_str();

//   // process control
//   // set calling thread name
//   // @note: the second argument just up to 16 bytes(including terminating null byte)
//   ::prctl(PR_SET_NAME, CurrentThread::t_name);
  
//   try {
//     func_();
//     CurrentThread::t_name = "finished";
//   } catch(std::exception const& ex) {
//     ::fprintf(stderr, "exception caught in Thread %s\n", name_.c_str());
//     ::fprintf(stderr, "reason: %s", ex.what());
//     ::fflush(stderr);
//     abort(); // don't throw or return just terminate
//   } catch(...) {
//     ::fprintf(stderr, "exception caught in Thread %s\n", name_.c_str());
//     KANON_RETHROW;
//   }
// }

// void* startThread(void* arg){
//   auto data = std::unique_ptr<ThreadData>{ static_cast<ThreadData*>(arg) };
//   data->run();

//   return nullptr;
// }

}//namespace detail

AtomicInt32 Thread::numCreated_{};

Thread::Thread(Threadfunc func, string const& name)
  : func_{std::move(func)},
    is_started_{false},
    is_joined_{false},
    pthreadId_{0},
    name_{name}
{
  numCreated_.increment();
  SetDefaultName();
}

Thread::~Thread(){
  if(is_started_ && !is_joined_) {
    pthread_detach(pthreadId_);
  }
}

void Thread::StartRun(){
  assert(!is_started_);
  is_started_ = true;
  
  // auto data = new detail::ThreadData{ func_, name_ };
  int err;
  // if ((err = pthread_create(&pthreadId_, NULL, &detail::startRun, data)) != 0) {
  if((err = pthread_create(&pthreadId_, NULL, &detail::Run, this)) != 0){
    is_started_ = false;
    // delete data;

    char buf[128];
    auto ret = ::strerror_r(errno, buf, sizeof buf);
    KANON_UNUSED(ret);
    ::fprintf(stderr, "Failed in pthread_create: %s", buf);
    ::fflush(stderr);
    exit(1);
  }
}

void Thread::Join(){
  assert(is_started_);
  assert(!is_joined_);

  is_joined_ = true;

  pthread_join(pthreadId_, NULL);
}

void Thread::SetDefaultName(){
  if(!name_.empty()) return ;
  
  char buf[64];
  BZERO(buf, sizeof buf);
  
  snprintf(buf, sizeof buf, "KanonThread%d", numCreated_.get());
  name_ = buf;
}

namespace detail {

void* Run(void* arg) {
  KANON_UNUSED(CurrentThread::tid());
  auto thread = reinterpret_cast<Thread*>(arg);
  CurrentThread::t_name = thread->name_.empty() ? "KanonThread" : thread->name_.c_str();

  // process control
  // set calling thread name
  // @note: the second argument just up to 16 bytes(including terminating null byte)
  ::prctl(PR_SET_NAME, CurrentThread::t_name);
  
  try {
    thread->func_();
    CurrentThread::t_name = "finished";
  } catch(std::exception const& ex) {
    ::fprintf(stderr, "exception caught in Thread %s\n", thread->name_.c_str());
    ::fprintf(stderr, "reason: %s", ex.what());
    ::fflush(stderr);
    abort(); // don't throw or return just terminate
  } catch(...) {
    ::fprintf(stderr, "exception caught in Thread %s\n", thread->name_.c_str());
    KANON_RETHROW;
  }

  return NULL;
}

} // namespace detail
} // namespace kanon