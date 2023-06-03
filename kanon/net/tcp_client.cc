#include "kanon/util/platform_macro.h"
#ifdef KANON_ON_WIN
#  include <winsock2.h>
#endif

#include "kanon/net/tcp_client.h"

#include "kanon/net/event_loop.h"
#include "kanon/net/inet_addr.h"
#include "kanon/net/connector.h"
#include "kanon/net/connection/tcp_connection.h"
#include "kanon/net/sock_api.h"
#include "kanon/net/user_client.h"
#include "kanon/net/channel.h"

using namespace kanon;

TcpClient::TcpClient(EventLoop *loop, InetAddr const &serv_addr,
                     std::string const &name)
  : loop_{loop} // Use std::make_shared is ok here since no weak pointer
  , connector_{Connector::NewConnector(loop, serv_addr)}
  , name_{name}
  , connection_callback_{&DefaultConnectionCallback} // , connect_{ true }
  , retry_{false}
  , conn_{nullptr}
  , mutex_{}
{
  LOG_TRACE_KANON << "TcpClient-[" << name_ << "]"
                  << " is constructed";
}

InetAddr const &TcpClient::GetServerAddr() const KANON_NOEXCEPT
{
  return connector_->GetServerAddr();
}

TcpClient::~TcpClient() KANON_NOEXCEPT
{
  LOG_TRACE_KANON << "TcpClient-[" << name_ << "]"
                  << " is destructed";

  bool is_unique = false;
  TcpConnectionPtr conn;
  {
    MutexGuard guard{mutex_};

    is_unique = (conn_.use_count() == 1); // not atomic

    // To write or read conn_ is not thread-safe,
    // we can copy it first, for the duplication,
    // it is thread-safe.
    // Because conn_ if is not released, conn also.
    // In the following operations, it is also alive
    // since ref count is not 0(i.e. pinned). Otherwise,
    // conn is released. we can't access it.
    conn = conn_;
  }

  LOG_INFO << "is_unique: " << is_unique;

  // Has established new connection
  if (conn) {
    assert(conn->GetLoop() == loop_);
    // Should not use old close callback
    // may be other thread using it
    if (is_unique) {
      conn->ForceClose();
    }
  } else {
    // Disable the connector
    connector_->Stop();
  }
}

void TcpClient::Connect() { connector_->StartRun(); }

void TcpClient::Disconnect()
{
  retry_ = false;
  // FIXME use weak callback?
  MutexGuard guard{mutex_};
  if (conn_) {
    conn_->ShutdownWrite();
  }
}

void TcpClient::Stop()
{
  retry_ = false;
  // in loop
  connector_->Stop();
}

void TcpClient::Init()
{
  connector_->SetNewConnectionCallback(
      std::bind(TcpClient::NewConnection, _1, GetServerAddr(),
                std::weak_ptr<TcpClient>(shared_from_this())));
}

/*
 * Must use std::weak_ptr to avoid reference cycle
 *
 * TcpClient contains std::shared_ptr<Connector>
 * Connector contains std::shared_ptr<TcpClient>
 */
void TcpClient::NewConnection(FdType sockfd, InetAddr const &serv_addr,
                              std::weak_ptr<TcpClient> const &wcli)
{
  /* Avoid reference cycle */
  auto cli = wcli.lock();

  if (!cli || cli->conn_) return;

  LOG_DEBUG_KANON << "cli sc = " << cli.use_count();

  LOG_TRACE_KANON << " New connection fd = " << sockfd;
  auto new_conn = TcpConnection::NewTcpConnection(
      cli->loop_, cli->name_, sockfd, sock::GetLocalAddr(sockfd), serv_addr);

  LOG_TRACE_KANON << "New connection: " << new_conn.get();
  // Must copy callback instead of moving them
  new_conn->SetMessageCallback(cli->message_callback_);
  new_conn->SetWriteCompleteCallback(cli->write_complete_callback_);
  new_conn->SetConnectionCallback(cli->connection_callback_);
#ifdef KANON_ON_WIN
  new_conn->SetChannel(cli->connector_->channel());
#endif

  // If we directly contruct and set new connection,
  // that is dangerous! we may be get a half-completed connection.
  {
    MutexGuard guard{cli->mutex_};
    cli->conn_ = std::move(new_conn);
  }

  // RemoveConnection
  //
  // The client shouldn't passed in std::shared_ptr reference or duplicate.
  // Pass it as weak_ptr and check it.
  cli->conn_->SetCloseCallback(std::bind(&TcpClient::CloseConnectionHandler,
                                         std::weak_ptr<TcpClient>(cli), _1));

  // enable reading
  cli->conn_->ConnectionEstablished();
}

TcpClientPtr kanon::NewTcpClient(EventLoop *loop, InetAddr const &serv_addr,
                                 std::string const &name, bool compact)
{
  auto ret =
      compact
          ? MakeSharedFromProtected<TcpClient>(loop, serv_addr, name)
          : std::shared_ptr<TcpClient>(new TcpClient(loop, serv_addr, name));
  ret->Init();
  return ret;
}

/*
 * Suppose `cli` is the object of `TcpClient` and `cli->conn` is the connection
 * member of it.
 * In some cases, other thread hold the duplicate of `cli->conn` but don't hold
 * the duplicate of `cli`.
 *
 * If main thread decrease the ref-count of `cli`, the client can't be destoryed
 * since the ref-count is greater than 1. we can't pass it be shard_ptr
 * duplicate.
 *
 * If pass it as reference, the client may be invalid in the callback.
 * If main thread destroy the client, and conn is not
 * unique. The close is called by other thread, and the close handler must be
 * valid.
 */
void TcpClient::CloseConnectionHandler(std::weak_ptr<TcpClient> const &wp_cli,
                                       TcpConnectionPtr const &conn)
{
  auto cli = wp_cli.lock();
  if (!cli) {
    return;
  }

  cli->loop_->AssertInThread();
  // passive close connection
  assert(conn == cli->conn_);
  assert(conn->GetLoop() == cli->loop_);

  // Like TcpServer, we remove connection from TcpClient
  {
    MutexGuard guard{cli->mutex_};
    cli->conn_.reset();
    LOG_INFO_KANON << conn->GetName() << " has removed";
  }

  // ! In event handling phase, don't remove channel(disable is allowed)
  // ! conn must be copied(\see TcoConnection::HandleClose())
  cli->loop_->QueueToLoop([conn]() {
    conn->ConnectionDestroyed();
  });

  // If user does not call Disconnect() and
  // support restart when passive close occurred
  if (cli->retry_) {
    cli->connector_->Restrat();
  }
}
