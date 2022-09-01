#include "chunk_stream.h"

using namespace kanon::protobuf;

bool ChunkStream::Next(void **data, int *size)
{
  Chunk *last_chunk = (chunk_list_.IsEmpty()) ? nullptr : chunk_list_.GetLastChunk();
  if (!last_chunk ||
      last_chunk->GetWritableSize() == 0)
  {
    chunk_list_.ReserveWriteChunk(1);
    last_chunk = chunk_list_.GetLastChunk();
    *data = last_chunk->GetWriteBegin();
    *size = ChunkList::CHUNK_SIZE;
  } else {
    *data = last_chunk->GetWriteBegin();
    *size = last_chunk->GetWritableSize();
    assert(*size == ChunkList::CHUNK_SIZE);
  }
  last_chunk->AdvanceWriteAll();
  return true;
}

void ChunkStream::BackUp(int count)
{
  auto last_chunk = chunk_list_.GetLastChunk();
  last_chunk->AdvanceWrite(-count);
}

int64_t ChunkStream::ByteCount() const
{
  return chunk_list_.GetReadableSize();
}

bool ChunkStream::WriteAliasedRaw(void const *data, int size) {
  chunk_list_.Append(data, size);
  return true;
}