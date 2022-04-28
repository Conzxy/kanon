#include "kanon/log/async_log.h"

#include "kanon/util/time_stamp.h"

#include "kanon/log/log_file.h"

using namespace kanon;

#define BUFFER_BOUND 16

AsyncLog::AsyncLog(
    StringView basename,
    size_t roll_size,
    StringView prefix,
    size_t log_file_num,
    size_t roll_interval,
    size_t flush_interval)
    : basename_{ basename.ToString() }
    , roll_size_{ roll_size }
    , prefix_{ prefix.ToString() }
    , log_file_num_(log_file_num)
    , roll_interval_(roll_interval)
    , flush_interval_{ flush_interval }
    , running_{ false }
    , current_buffer_{ kanon::make_unique<Buffer>() }
    , next_buffer_{ kanon::make_unique<Buffer>() }
    , mutex_{ }
    , not_empty_{ mutex_ }
    , back_thr_{ "AsyncLog" }
    , latch_{ 1 }
{
  // warm up
  current_buffer_->zero();
  next_buffer_->zero();
  buffers_.reserve(16);

  Logger::SetColor(false); 
}

AsyncLog::~AsyncLog() noexcept {
  if (running_) {
    Stop();
  }
}

void AsyncLog::StartRun() {
  assert(!running_);

  running_ = true;

  buffers_dup_.reserve(16);

  back_thr_.StartRun([this]() {
    latch_.Countdown();

    // In back thread, we also can set two buffer for front threads using,
    BufferUPtr buffer1{ kanon::make_unique<Buffer>() };
    BufferUPtr buffer2{ kanon::make_unique<Buffer>() };

    // Warm up
    buffer1->zero();
    buffer2->zero();  

    // write to disk
    // Not thread-safe is OK here.
    LogFile<> output(basename_, roll_size_, 
      prefix_, log_file_num_, roll_interval_, flush_interval_);

    // back thread do long loop
    while (running_) {
      assert(buffer1 && buffer1->len() == 0);
      assert(buffer2 && buffer2->len() == 0);
      assert(buffers_dup_.empty());

      {
        MutexGuard guard{ mutex_ };
        // \warning 
        //   You shouldn't use while.
        //   Otherwise, it will wait infinitly
        //   when current message accumulation
        //   does not reach a buffer size.
        // \note
        //   This is not a classic use,
        //   but there is one cosumer, use if here is safe
        if (buffers_.empty()) {
          // If front thread log message is so short,
          // we also awake and write
          // To ensure real time message
          not_empty_.WaitForSeconds(flush_interval_);
        }

        // Although current_buffer_ have space to fill
        // we also push it to buffers_
        buffers_.emplace_back(std::move(current_buffer_));

        buffers_dup_.swap(buffers_);

        current_buffer_ = std::move(buffer1);
        if (!next_buffer_) {
          next_buffer_ = std::move(buffer2);
        }
      }

      // If buffer total size over 64M,
      // we discard these part to avoid accumulation
      if (buffers_dup_.size() > BUFFER_BOUND) {
        char buf[64];
        ::snprintf(
          buf, sizeof buf, "Discard %lu large buffer at %s\n",
          buffers_dup_.size() - BUFFER_BOUND, 
          TimeStamp::Now().ToFormattedString().c_str());
        // ::fputs(buf, stderr);
        output.Append(buf, strlen(buf));

        buffers_dup_.erase(buffers_dup_.begin() + BUFFER_BOUND, buffers_dup_.end());
      }  

      for (auto& buffer : buffers_dup_) {
        output.Append(buffer->data(), buffer->len());
      }
    
      // only leave the two warmed buffer
      // and move to buffer1 and buffer2
      if (buffers_dup_.size() > 2)
        buffers_dup_.resize(2);

      if (!buffer1) {
        buffer1 = std::move(buffers_dup_.back());
        // reuse buffer
        buffer1->reset();
        buffers_dup_.pop_back();
      }

      if (!buffer2) {
        buffer2 = std::move(buffers_dup_.back());
        buffer2->reset();
        buffers_dup_.pop_back();
      }
      
      buffers_dup_.clear();

      output.Flush();
    }

    // Flush output buffer(the last)
    output.Flush();
  });

  // Main thread wait until back thread runs
  latch_.Wait();
}

void AsyncLog::Stop() noexcept {
  assert(running_);

  running_ = false;
  not_empty_.Notify();
  back_thr_.Join();
}

void AsyncLog::Append(char const* data, size_t len) noexcept {
    MutexGuard guard{ mutex_ };
  
    // If there are no space for data in @var current_buffer_,
    // swap with @var next_buffer_, and append new buffer
    // then notify @var back_thr_ to log message
    if (len < current_buffer_->avali()) {
      current_buffer_->Append(data, len);
    } else {
      buffers_.emplace_back(std::move(current_buffer_));

      if (!next_buffer_) {
        next_buffer_.reset(new Buffer{});
      }

      current_buffer_ = std::move(next_buffer_);
      assert(!next_buffer_);
      current_buffer_->Append(data, len);
      not_empty_.Notify();
    }
}

void AsyncLog::Flush() noexcept {
  // The flush operation is called in back thread
}