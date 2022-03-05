#ifndef KANON_PROTOBUF_PROTOBUF_CODEC_H
#define KANON_PROTOBUF_PROTOBUF_CODEC_H

#include "generic_pb_codec.h"

#include <type_traits>

#include <google/protobuf/message.h>

#include "kanon/util/ptr.h"

namespace kanon {
namespace protobuf {

template<typename M, char const* T>
class ProtobufCodec : noncopyable {
  static_assert(std::is_base_of<PROTOBUF::Message, M>::value,
      "M(Message) template parameter must be the derived class of ::google::protobuf::Messgae");
  
  using Codec = internal::GenericPbCodec; 
  using ConcreteMessage = M; 
  // using ConcreteMessagePtr = std::shared_ptr<M>;
  using ConcreteMessagePtr = ConcreteMessage*;

  using MessageCallback = 
    std::function<void(TcpConnectionPtr const&, ConcreteMessagePtr, TimeStamp)>;

  using ErrorCallback = Codec::ErrorCallback;
public:
  /**
   * @warning 
   * ConcreteMessage::default_instance() return type is ConcreteMessage&
   * but in @link https://developers.google.com/protocol-buffers/docs/reference/cpp/google.protobuf.message?hl=en#MessageFactory.GetPrototype, the invariant in MessageFactory::generated_factory(), give the following:
   * MessageFactory::generated_factory()->GetPrototype(
   * FooMessage::descriptor()) == FooMessage::default_instance()
   * It is incorrect invariant.
   */
  ProtobufCodec()
    : generic_codec_(&M::default_instance(), T)
  {
    generic_codec_.SetMessageCallback(
      [this](TcpConnectionPtr const& conn, MessagePtr message, TimeStamp receive_time)
      {
        message_callback_(conn, kanon::down_pointer_cast<ConcreteMessage>(message), receive_time);
      }
    );
  }
  
  void Send(TcpConnectionPtr const& conn, PROTOBUF::Message const* message)
  { generic_codec_.Send(conn, message); }
  
  void SetErrorCallback(ErrorCallback cb) noexcept { generic_codec_.SetErrorCallback(std::move(cb)); }
  void SetMessageCallback(MessageCallback cb) noexcept { message_callback_ = std::move(cb); }

  void OnMessage(TcpConnectionPtr const& conn, Buffer& buffer, TimeStamp receive_time)
  { generic_codec_.OnMessage(conn, buffer, receive_time); }
  
  ~ProtobufCodec() = default; 
private:
  Codec generic_codec_;
  MessageCallback message_callback_;
};

} // namespace protobuf
} // namespace kanon

#endif // KANON_PROTOBUF_PROTOBUF_CODEC_H
