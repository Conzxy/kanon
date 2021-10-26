#include "kanon/log/AsyncLog.h"

#include "kanon/log/LogFile.h"
#include "kanon/time/TimeStamp.h"

using namespace kanon;

#define BUFFER_BOUND 16

AsyncLog::AsyncLog(
    StringView basename,
    size_t rollSize,
    size_t flushInterval,
    size_t rollLine)
    : basename_{ basename }
    , rollSize_{ rollSize }
    , flushInterval_{ flushInterval }
    , rollLine_{ rollLine }
	, running_{ false }
	, currentBuffer_{ kanon::make_unique<Buffer>() }
	, nextBuffer_{ kanon::make_unique<Buffer>() }
    , mutex_{ }
    , notEmpty_{ mutex_ }
    , backThread_{ [this]() {
        this->threadFunc();
    } }
	, latch_{ 1 }
{
	// warm up
	currentBuffer_->zero();
	nextBuffer_->zero();
	buffers_.reserve(16);

}

AsyncLog::~AsyncLog() KANON_NOEXCEPT {
	if (running_) {
		stop();
	}
}

void
AsyncLog::start() {
	assert(!running_);

	running_ = true;
	backThread_.start();

	// Main thread wait until back thread runs
	latch_.wait();
}

void 
AsyncLog::stop() KANON_NOEXCEPT {
	assert(running_);

	running_ = false;
	notEmpty_.notify();
	backThread_.join();
}

void
AsyncLog::append(char const* data, size_t len) KANON_NOEXCEPT {
    MutexGuard guard{ mutex_ };
	
	// If there are no space for data in @var currentBuffer_,
	// swap with @var nextBuffer_, and append new buffer
	// then notify @var backThread_ to log message
    if (len < currentBuffer_->avali()) {
		currentBuffer_->append(data, len);
    } else {
		buffers_.emplace_back(std::move(currentBuffer_));

        if (!nextBuffer_) {
			nextBuffer_.reset(new Buffer{});
		}

		currentBuffer_ = std::move(nextBuffer_);

		currentBuffer_->append(data, len);
		notEmpty_.notify();
    }
}

void
AsyncLog::flush() KANON_NOEXCEPT {
	// flush operation is called in back thread
	// Logger do nothing	
}

void
AsyncLog::threadFunc() {
	// In back thread, we also can set two buffer for front threads using,
	latch_.countdown();
	
	BufferUPtr buffer1{ kanon::make_unique<Buffer>() };
	BufferUPtr buffer2{ kanon::make_unique<Buffer>() };

	// warm up
	buffer1->zero();
	buffer2->zero();	

	// write to disk
	LogFile<> output{ basename_, rollSize_, flushInterval_, rollLine_ };
	
	Buffers tmpBuffers;

	// back thread do infinite loop
	while (running_) {
		assert(buffer1 && buffer1->len() == 0);
		assert(buffer2 && buffer2->len() == 0);
		assert(tmpBuffers.empty());

		tmpBuffers.reserve(16);	
		{
			MutexGuard guard{ mutex_ };
			// @warning 
			// You shouldn't use while.
			// Otherwise, it will wait infinitly
			// when current message accumulation
			// does not reach a buffer size.
			// @note
			// this is not a classic use,
			// but there are one cosumer, so you can just use if here.
			if (buffers_.empty()) {
				// If front thread log message is so short,
				// we also awake and write
				// To ensure real time message
				notEmpty_.waitForSeconds(flushInterval_);
			}

			// Although @var currentBuffer_ have space to fill
			// we also push it to @var buffers_
			buffers_.emplace_back(std::move(currentBuffer_));

			// Since tmpBuffers is local, it must be thread safe
			tmpBuffers.swap(buffers_);

			currentBuffer_ = std::move(buffer1);
			if (!nextBuffer_) {
				nextBuffer_ = std::move(buffer2);
			}
		} // block end

		// If buffer total size over 64M,
		// we discard these part to avoid accumulation
		if (tmpBuffers.size() > BUFFER_BOUND) {
			char buf[64];
			::snprintf(
				buf, sizeof buf, "Discard %lu large buffer at %s\n",
				tmpBuffers.size() - BUFFER_BOUND, 
				TimeStamp::now().toFormattedString().c_str());
			fputs(buf, stderr);
			output.append(buf, strlen(buf));

			tmpBuffers.erase(tmpBuffers.begin() + BUFFER_BOUND, tmpBuffers.end());
		}	

		for (auto& buffer : tmpBuffers) {
			output.append(buffer->data(), buffer->len());
		}
	
		// only leave the two warmed buffer
		// and move to buffer1 and buffer2
		if (tmpBuffers.size() > 2)
			tmpBuffers.resize(2);

		if (!buffer1) {
			buffer1 = std::move(tmpBuffers.back());
			// reuse buffer
			buffer1->reset();
			tmpBuffers.pop_back();
		}

		if (!buffer2) {
			buffer2 = std::move(tmpBuffers.back());
			buffer2->reset();
			tmpBuffers.pop_back();
		}
		
		tmpBuffers.clear();

		output.flush();
	}
	// flush output buffer(the last)
	output.flush();
}
