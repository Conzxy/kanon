#include "kanon/protobuf/chunk_stream.h"

#include <iostream>

#include "../rpc/pb/echo.pb.h"

using namespace kanon::protobuf;

char buf[4096 * 10];

int main()
{
  for (size_t i = 0; i < sizeof buf; ++i) {
    buf[i] = '0';
  }

  ChunkStream stream;
  EchoArgs args;
  args.set_msg(buf);

  args.SerializeToZeroCopyStream(&stream);

  auto &chunk_list = stream.chunk_list;
  for (auto &chunk : stream.chunk_list) {
    std::cout << "readable: " << chunk.GetReadableSize()
              << "\nwritable: " << chunk.GetWritableSize()
              << "\nmax: " << chunk.GetMaxSize() << std::endl;
  }

  auto end = chunk_list.GetChunkSize();
  auto beg = chunk_list.begin();
  for (size_t i = 0; i < end - 1; ++i) {
    beg->AdvanceWriteAll();
    ++beg;
  }

  std::cout << "After: \n";
  for (auto &chunk : stream.chunk_list) {
    std::cout << "readable: " << chunk.GetReadableSize()
              << "\nwritable: " << chunk.GetWritableSize()
              << "\nmax: " << chunk.GetMaxSize() << std::endl;
  }

  std::cout << "readable size: " << chunk_list.GetReadableSize() << "\n";
  std::cout << "cached size: " << args.ByteSizeLong() << std::endl;

  kanon::ChunkList chunk_list2;
  chunk_list2.Append(buf, 4096);
  chunk_list2.Append(buf, 0);
}
