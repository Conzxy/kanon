#include "kanon/thread/thread.h"

#ifdef KANON_ON_UNIX
#include <sys/prctl.h>
#elif defined(KANON_ON_WIN)
#include <windows.h>
#include <processthreadsapi.h>
#endif

#include <stdio.h>
#include <string.h>
#include <assert.h>

#include <system_error>
// #include "kanon/thread/atomic.h"
#include "kanon/thread/atomic_counter.h"
#include "kanon/thread/current_thread.h"

#include "kanon/util/noncopyable.h"
#include "kanon/util/macro.h"
#include "kanon/util/ptr.h"

using namespace std;

namespace kanon {

AtomicCounter32 Thread::numCreated_{};

Thread::Thread(std::string const &name)
  : Thread(Threadfunc(), name)
{
}

Thread::Thread(Threadfunc func, string const &name)
  : func_{std::move(func)}
  , is_started_{false}
  , is_joined_{false}
  , pthreadId_{0}
  , name_{name}
{
  numCreated_.Add(1);
  SetDefaultName();
}

Thread::~Thread()
{
  if (is_started_ && !is_joined_) {
#ifdef KANON_ON_UNIX
    ::pthread_detach(pthreadId_);
#else
    thr_.detach();
#endif
  }
}

void Thread::StartRun()
{
  assert(!is_started_);
  is_started_ = false;

  // auto data = new detail::ThreadData{ func_, name_ };
#ifdef KANON_ON_UNIX
  int err = 0;
  err = pthread_create(&pthreadId_, NULL, &detail::Run, this);
  if (err != 0) {
    char buf[128];
    auto ret = ::strerror_r(errno, buf, sizeof buf);
    KANON_UNUSED(ret);
    ::fprintf(stderr, "Failed in pthread_create: %s\n", buf);
    ::fflush(stderr);
    exit(1);
  } else {
    is_started_ = true;
  }
#else
  try {
    std::thread thr(&detail::Run, this);
#ifdef KANON_ON_UNIX
    pthreadId_ = thr.native_handle();
#else
    pthreadId_ = GetThreadId(thr.native_handle());
#endif
    thr_ = std::move(thr);
    is_started_ = true;
  }
  catch (std::system_error const &ex) {
    fprintf(stderr, "Failed to create thread: %s!\n", ex.what());
    exit(1);
  }
  catch (...) {
    fprintf(stderr, "Failed to create thread for unknown reason!");
    exit(1);
  }
#endif
  // if ((err = pthread_create(&pthreadId_, NULL, &detail::startRun, data)) !=
  // 0) {
}

void Thread::StartRun(Threadfunc cb)
{
  func_ = std::move(cb);
  StartRun();
}

void Thread::Join()
{
  assert(is_started_);
  assert(!is_joined_);

  is_joined_ = true;

#ifdef KANON_ON_UNIX
  pthread_join(pthreadId_, NULL);
#else
  thr_.join();
#endif
}

void Thread::SetDefaultName()
{
  if (!name_.empty()) return;

  char buf[64];
  BZERO(buf, sizeof buf);

  snprintf(buf, sizeof buf, "KanonThread%d", numCreated_.GetValue());
  name_ = buf;
}

namespace detail {

void *Run(void *arg)
{
  CurrentThread::cacheTid();
  auto thread = reinterpret_cast<Thread *>(arg);
  CurrentThread::t_name =
      thread->name_.empty() ? "KanonThread" : thread->name_.c_str();

  // process control
  // set calling thread name
  // \note: the second argument just up to 16 bytes(including terminating null
  // byte)
#ifdef KANON_ON_UNIX
  ::prctl(PR_SET_NAME, CurrentThread::t_name);
#else
  // TODO set thread name in WINDOWS
#endif

  try {
    thread->func_();
    CurrentThread::t_name = "finished";
  }
  catch (std::exception const &ex) {
    ::fprintf(stderr, "std::exception is caught in Thread %s\n",
              thread->name_.c_str());
    ::fprintf(stderr, "Reason: %s", ex.what());
    ::fflush(stderr);
    ::abort(); // don't throw or return just terminate
  }
  catch (...) {
    ::fprintf(stderr, "Unknown exception is caught in Thread %s\n",
              thread->name_.c_str());
    ::abort();
  }

  return NULL;
}

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
//   // \note: the second argument just up to 16 bytes(including terminating
//   null byte)
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

} // namespace detail
} // namespace kanon
