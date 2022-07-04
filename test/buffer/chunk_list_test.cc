#include "kanon/buffer/chunk_list.h"

#include <gtest/gtest.h>

#include "kanon/log/logger.h"

using namespace kanon;

static constexpr unsigned BUF_SIZE = 65536;
static char g_buf[BUF_SIZE];
inline int InitGBuf() {
  static char const hex[] = "0123456789abcdef";

  for (unsigned i = 0; i < sizeof(g_buf); ++i) {
    g_buf[i] = hex[i&0xf];
  }

  return 0;
}

static int g_dummy = InitGBuf();

static unsigned RoundChunkSize(size_t len, size_t chunk_size) noexcept {
  assert((chunk_size & 0x1) == 0);

  if ((len & (chunk_size-1)) == 0) {
    return len / chunk_size;
  } else {
    return len / chunk_size + 1;
  }
}

TEST(chunk_list, chunk_size_round) {
  EXPECT_EQ(RoundChunkSize(4099, 4096), 2);
  EXPECT_EQ(RoundChunkSize(8192, 4096), 2);
  EXPECT_EQ(RoundChunkSize(4096, 4096), 1);
}

TEST(chunk_list, Append) {
  ChunkList chunks;

  chunks.Append(g_buf, sizeof g_buf);
  
  auto expected_chunk_size = BUF_SIZE / ChunkList::GetSingleChunkSize();
  EXPECT_EQ(chunks.GetChunkSize(), expected_chunk_size);
  EXPECT_FALSE(chunks.IsEmpty());

  auto first_chunk = chunks.GetFirstChunk();
  EXPECT_EQ(first_chunk->GetReadableSize(), ChunkList::GetSingleChunkSize());

  // ********** Test Advance *************
  chunks.AdvanceRead(4097);

  expected_chunk_size -= 1;
  EXPECT_EQ(chunks.GetChunkSize(), expected_chunk_size);
  EXPECT_EQ(chunks.GetFreeChunkSize(), 1);

  first_chunk = chunks.GetFirstChunk();
  EXPECT_EQ(first_chunk->GetReadableSize(), ChunkList::GetSingleChunkSize() - 1);

  chunks.AdvanceRead(4097);
  expected_chunk_size -= 1;

  EXPECT_EQ(chunks.GetChunkSize(), expected_chunk_size);
  EXPECT_EQ(chunks.GetFreeChunkSize(), 2);
  first_chunk = chunks.GetFirstChunk();
  EXPECT_EQ(first_chunk->GetReadableSize(), ChunkList::GetSingleChunkSize() - 2);
  auto last_chunk = chunks.GetLastChunk();
  EXPECT_EQ(last_chunk->GetWritableSize(), 0);

  chunks.Append(g_buf, 4099);

  expected_chunk_size += 2;
  EXPECT_EQ(chunks.GetChunkSize(), expected_chunk_size);
  EXPECT_EQ(chunks.GetFreeChunkSize(), 0);
}

TEST(chunk_list, Prepend) {
  ChunkList buffer;

  puts("===== Prepend8 =====");
  buffer.Prepend8(32);
  EXPECT_EQ(buffer.Read8(), 32);

  puts("===== Prepend16 =====");
  buffer.Prepend16(1 << 12);
  EXPECT_EQ(buffer.Read16(), 1 << 12);

  puts("===== Prepend32 ======");
  buffer.Prepend32(1 << 24);
  EXPECT_EQ(buffer.Read32(), 1 << 24);

  puts("===== Prepend64 =====");
  uint64_t i = static_cast<uint64_t>(1) << 33;
  buffer.Prepend64(i);
  EXPECT_EQ(buffer.Read64(), uint64_t(1) << 33);

  ChunkList buffer2;

  buffer2.Append("Conzxy KANON");
  buffer2.Prepend16(buffer2.GetReadableSize());

  buffer2.WriteFd(STDOUT_FILENO);

  EXPECT_EQ(buffer2.GetChunkSize(), 2);
  EXPECT_EQ(buffer2.GetReadableSize(), 14);
  EXPECT_EQ(buffer2.Read16(), buffer2.GetReadableSize() - sizeof(uint16_t));
  EXPECT_EQ(buffer2.GetChunkSize(), 1);
  EXPECT_TRUE(buffer2.GetFirstChunk()->ToStringView() == "Conzxy KANON");

  LOG_DEBUG << buffer2.GetFirstChunk()->ToStringView();
  buffer2.AdvanceReadAll();
  EXPECT_TRUE(buffer2.IsEmpty());
}

TEST(chunk_list, move) {
  ChunkList buffer;

  buffer.Append("Conzxy KANON");
  buffer.Prepend16(buffer.GetReadableSize());

  auto buffer2 = std::move(buffer);
  EXPECT_EQ(buffer2.GetChunkSize(), 2);
  EXPECT_EQ(buffer2.GetReadableSize(), 14);
  EXPECT_EQ(buffer2.Read16(), buffer2.GetReadableSize() - sizeof(uint16_t));
  EXPECT_TRUE(buffer2.GetFirstChunk()->ToStringView() == "Conzxy KANON");  
  buffer2.AdvanceReadAll();
  EXPECT_TRUE(buffer2.IsEmpty());
}
TEST(chunk_list, Write) {
  ChunkList buffer;

  buffer.Append(g_buf, sizeof g_buf);

  // EXPECT_EQ(sizeof g_buf, buffer.WriteFd(STDOUT_FILENO));
}

TEST(chunk_list, MultiAppend) {
  ChunkList buffer;
  auto count = 4096 / 16; 

  for (int i = 0; i < 16; ++i) {
    buffer.Append(g_buf, count);
    printf("size = %zu\n", buffer.GetReadableSize());
  }

  EXPECT_EQ(buffer.GetReadableSize(), 4096);
  EXPECT_EQ(buffer.GetChunkSize(), 1);
}

int main() {
  ::testing::InitGoogleTest();
  return RUN_ALL_TESTS();
}
