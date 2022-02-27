#include "acceptor.h"
#include "event_loop.h"
#include "kanon/time/time_stamp.h"

using namespace kanon;

Acceptor::Acceptor(EventLoop* loop, InetAddr const& addr, bool reuseport)
  : loop_{ loop }
  , socket_{ sock::createNonBlockAndCloExecSocket(!addr.IsIpv4()) }
  , channel_{ loop_, socket_.GetFd() }
  , listening_{ false }
  , dummyfd_{ ::open("/dev/null", O_RDONLY | O_CLOEXEC) }
{
  socket_.setReuseAddr(true);
  socket_.setReusePort(reuseport);
  socket_.SetNoDelay(true);
  socket_.bindAddress(addr);
  
  // set listen channel read callback(accept peer end)
  // and start observing the read event  
  channel_.SetReadCallback([this](TimeStamp stamp) {
    KANON_UNUSED(stamp);
    loop_->AssertInThread();
    
    InetAddr cli_addr;
    auto cli_fd = socket_.accept(cli_addr);
    
    if (cli_fd >= 0) {
      if (new_connection_callback_) {
        // dispatching connection to IO thread
        new_connection_callback_(cli_fd, cli_addr);
      } else {
        sock::close(cli_fd);
      }
    } else {
    // if process limited open fd has reached,
    // os also accept this fd and put to wait queue
    // since we take level trigger policy,
    // so it will cause busy loop
    // so we should use dummy fd to accept and close it
      if (errno == EMFILE) {
        // first close dummy fd, leave a space for fd in wait queue
        ::close(dummyfd_);
        // accept the fd
        dummyfd_ = ::accept(socket_.GetFd(), NULL, NULL);
        ::close(dummyfd_);
        // create new dummy fd to use after
        dummyfd_ = ::open("/dev/null", O_RDONLY | O_CLOEXEC);  
      }

      // We don't handle error since sock::accept() has handled
    }
  });

  channel_.EnableReading();
}

Acceptor::~Acceptor() noexcept {
  listening_ = false;
  // FIXME: need DisableAll()?
  channel_.DisableAll();
  channel_.Remove();
  ::close(dummyfd_);
}

void
Acceptor::Listen() noexcept {
  assert(!listening_);
  sock::listen(socket_.GetFd());

  listening_ = true;
}
