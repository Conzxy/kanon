#ifndef KANON_RPC_CODEC_H__
#define KANON_RPC_CODEC_H__

#include "kanon/protobuf/protobuf_codec.h"
#include "kanon/rpc/rpc.pb.h"

namespace kanon {
namespace protobuf {
namespace rpc {

extern char const krpc_tag[];

using RpcCodec = ProtobufCodec<RpcMessage, krpc_tag>;

} // namespace rpc
} // namespace protobuf
} // namespace kanon
#endif // KANON_RPC_CODEC_H__
