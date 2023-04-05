#include "kanon/net/connector.h"

#include <winsock2.h>
#include <mswsock.h>

#include "kanon/net/event.h"
#include "kanon/net/channel.h"
#include "kanon/net/event_loop.h"
#include "kanon/net/sock_api.h"
#include "kanon/win/net/completion_context.h"

using namespace kanon;

void Connector::Connect()
{
  loop_->AssertInThread();
  if (state_ != kDisconnected) {
    return;
  }

  auto sockfd = sock::CreateOverlappedSocket(!serv_addr_.IsIpv4());

  channel_ = kanon::make_unique<Channel>(loop_, sockfd);
  channel_->EnableWriting();

  auto completion_ctx = new CompletionContext;
  completion_context_init(completion_ctx);
  completion_ctx->event = Event::WriteEvent;
  channel_->RegisterCompletionContext(completion_ctx);

  auto ret = sock::WinConnect(sockfd, serv_addr_.ToSockaddr(), completion_ctx);

  if (!ret) {
    auto saved_errno = ::WSAGetLastError();
    switch (saved_errno) {
      case WSA_IO_PENDING:
        // case WSAEINPROGRESS:
        // case WSAEINTR:
        LOG_TRACE_KANON << "Connect inprogress";
        CompleteConnect(sockfd);
        break;
      case WSAECONNREFUSED:
      case WSAENETUNREACH:
      case WSAETIMEDOUT:
        LOG_TRACE_KANON << "Failed to connect but retry";
        Retry(sockfd);
        break;
      default:
        LOG_SYSERROR_KANON << "Failed to call ConnectEx()";
        sock::Close(sockfd);
        ResetChannel();
        ::WSACleanup();
    }
  }
}

void Connector::CompleteConnect(FdType sockfd)
{
  KANON_ASSERT(state_ == State::kDisconnected && connect_,
               "Connection must be down and Stop isn't called(in loop)");

  /* Wait event occur */
  SetState(State::kConnecting);

  // assert(!channel_);

  channel_->SetWriteCallback(
      [this]() {
        FdType sockfd = channel_->GetFd();
        // NOTICE:
        // OUT maybe occurs with ERR(and HUP).
        // Therefore, the error callback is called before this,
        // use state to distinguish them.
        if (state_ == State::kConnecting) {
          ResetChannel();
          int err = sock::GetSocketError(sockfd);
          int seconds = 0;
          int bytes = sizeof(seconds);
          auto iResult = getsockopt(sockfd, SOL_SOCKET, SO_CONNECT_TIME,
                                    (char *)&seconds, (PINT)&bytes);
          bool can_retry = false;
          if (iResult != NO_ERROR) {
            LOG_SYSERROR_KANON << "Failed to call getsockopt(SO_CONNECT_TIME)";
            can_retry = true;
          } else {
            if (seconds == (0xffffffff)) {
              LOG_DEBUG_KANON << "Connection isn't established!";
              can_retry = true;
            } else {
              LOG_DEBUG_KANON << "Connection has been established " << seconds
                              << " seconds";
            }
          }

          if (can_retry) {
            Retry(sockfd);
            return;
          }

          int flag = 1;
          if (setsockopt(sockfd, SOL_SOCKET, SO_UPDATE_CONNECT_CONTEXT,
                         (char const *)&flag, sizeof(flag)))
          {
            LOG_SYSERROR_KANON
                << "Failed to setsockopt(SO_UPDATE_CONNECT_CONTEXT)";
          }

          if (err) {
            // Fatal errors have handled in Connect()
            LOG_WARN_KANON << "SO_ERROR = " << err << " " << strerror_tl(err);
            LOG_SYSERROR_KANON << "GetOverlappedResult(): ";
            Retry(sockfd);
          } else if (sock::IsSelfConnect(sockfd)) {
            // Self connection happend only when:
            // 1. The IP of client is same with server
            // 2. The port is a ephemeral port(selected by client kernel)
            // 3. Only occrurs in three-way shake initailization
            //
            // The self connection is so strange and hard to debug it.
            // We should refuse it.
            //
            // \see /proc/sys/net/ipv4/ip_local_port_range
            // \see
            // https://github.com/pirDOL/kaka/blob/master/Miscellaneous/TCP-client-self-connect.md
            LOG_WARN_KANON << "self connect";
            // Discard the client address and retry
            // until self connection is skipped
            Retry(sockfd);
          } else {
            // new_connection_callback should be seted by client
            SetState(State::kConnected);
            if (connect_) {
              if (new_connection_callback_) {
                new_connection_callback_(sockfd);
              } else {
                LOG_WARN_KANON
                    << "There is no new connection callback, cannot to create "
                       "connection";
              }
              LOG_TRACE_KANON << "Connection is established successfully";
            } else {
              // Must after Stop() is called
              LOG_WARN_KANON << "Cannot to complete connect since user stop it";
              sock::Close(sockfd);
            }
          }
        }
      });

  channel_->SetErrorCallback([this]() {
    if (state_ == State::kConnecting) {
      FdType sockfd = channel_->GetFd();
      ResetChannel();
      int err = sock::GetSocketError(sockfd);
      if (err) {
        LOG_ERROR_KANON << "SO_ERROR = " << err << " " << strerror_tl(err);
      }

      Retry(sockfd);
    }
  });
}
