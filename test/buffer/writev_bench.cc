#include <sys/uio.h>
#include <unistd.h>
#include <fcntl.h>

#include <benchmark/benchmark.h>

using namespace benchmark;

static char buf[65536];
static char bufs[16][4096];

static void writev_bench(State& state) {
  int fd = ::open("/dev/null", O_WRONLY);
  for (auto _ : state) {
    std::array<struct iovec, 16> buf_vec;
    for (unsigned i = 0; i < buf_vec.size(); ++i) {
      buf_vec[i].iov_base = bufs[i];
      buf_vec[i].iov_len = sizeof(bufs[i]);
    }

    ::writev(fd, buf_vec.begin(), buf_vec.size());
  }
  ::close(fd);
}

static void write_bench(State& state) {
  int fd = ::open("/dev/null", O_WRONLY);
  for (auto _ : state) {
    ::write(fd, buf, sizeof buf);
  }

  ::close(fd);
}

static void loop_write_bench(State& state) {
  int fd = ::open("/dev/null", O_WRONLY);

  for (auto _ : state) {
    for (unsigned i = 0; i < 16; ++i) {
      ::write(fd, bufs[i], sizeof bufs[i]);
    }
  }

  ::close(fd);
}

static void once_write_bench(State& state) {
  int fd = ::open("/dev/null", O_WRONLY);
  bool x = true;
  for (auto _ : state) {
    if (x) {
      ::write(fd, buf, 4096);
    }
  }

  ::close(fd);
}

static void once_writev_bench(State& state) {
  int fd = ::open("/dev/null", O_WRONLY);

  struct iovec vecs[1];
  vecs[0].iov_base = buf;
  vecs[0].iov_len = 4096;

  for (auto _ : state) {
    ::writev(fd, vecs, 1);
  }

  ::close(fd);
}

BENCHMARK(loop_write_bench);
BENCHMARK(writev_bench);
BENCHMARK(write_bench);
BENCHMARK(once_write_bench);
BENCHMARK(once_writev_bench);
BENCHMARK_MAIN();

// In my machine, can conclude follwing:
// 1) The writev >> loop write(decrease syscall)
// 2) The write * 2 == writev but can accept
// 3) To one chunk, the write() is better than writev() also