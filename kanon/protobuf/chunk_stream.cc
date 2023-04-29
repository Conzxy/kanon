#include "chunk_stream.h"

using namespace kanon::protobuf;

bool ChunkStream::Next(void **data, int *size)
{
  Chunk *last_chunk =
      (chunk_list.IsEmpty()) ? nullptr : chunk_list.GetLastChunk();
  if (!last_chunk || last_chunk->GetWritableSize() == 0) {
    chunk_list.ReserveChunk(1);
    last_chunk = chunk_list.GetLastChunk();
  }
  *data = last_chunk->GetWriteBegin();
  *size = last_chunk->GetWritableSize();
  last_chunk->AdvanceWriteAll();
  return true;
}

void ChunkStream::BackUp(int count)
{
  auto last_chunk = chunk_list.GetLastChunk();
  last_chunk->AdvanceWrite(-count);
}

int64_t ChunkStream::ByteCount() const { return chunk_list.GetReadableSize(); }

bool ChunkStream::WriteAliasedRaw(void const *data, int size)
{
  chunk_list.Append(data, size);
  return true;
}
