#include "kanon/net/acceptor.h"

#include "kanon/util/check.h"
#include "kanon/util/time_stamp.h"

#include "kanon/net/event_loop.h"

using namespace kanon;

Acceptor::Acceptor(EventLoop* loop, InetAddr const& addr, bool reuseport)
  : loop_{ loop }
  , socket_{ sock::CreateNonBlockAndCloExecSocket(!addr.IsIpv4()) }
  , channel_{ loop_, socket_.GetFd() }
  , listening_{ false }
  , dummyfd_{ ::open("/dev/null", O_RDONLY | O_CLOEXEC) }
{
  socket_.SetReuseAddr(true);
  socket_.SetReusePort(reuseport);
  socket_.BindAddress(addr);
  
  // set listen channel read callback(accept peer end)
  // and start observing the read event  
  channel_.SetReadCallback([this](TimeStamp stamp) {
    KANON_UNUSED(stamp);
    loop_->AssertInThread();
    
    InetAddr cli_addr;
    auto cli_fd = socket_.Accpet(cli_addr);
    
    if (cli_fd >= 0) {
      if (new_connection_callback_) {
        // dispatching connection to IO thread
        new_connection_callback_(cli_fd, cli_addr);
      } else {
        sock::Close(cli_fd);
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

      // We don't handle error since sock::Accept() has handled
    }
  });

  // Can't call Channel::EnableReading() in the ctor
  // Construct a server in the other loop is allowed.
  // e.g.
  // EventLoopThread loop_thr;
  // TcpServer server(loop_thr.StartRun(), addr); // Oops!
  // server.StartRun();
}

Acceptor::~Acceptor() noexcept {
  // FIXME 
  loop_->AssertInThread();

  listening_ = false;
  // FIXME Need DisableAll()?
  channel_.DisableAll();
  channel_.Remove();
  ::close(dummyfd_);
}

void Acceptor::Listen() noexcept {
  assert(!listening_);
  loop_->AssertInThread();
  
  sock::Listen(socket_.GetFd());
  listening_ = true;
  channel_.EnableReading();
}
