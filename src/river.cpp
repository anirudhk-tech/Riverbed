#include "Riverbed/river.hpp"

#include <cstdint>
#include <cstddef>
#include <cassert>
#include <memory>

namespace Riverbed {

River::River(size_t capacity):
  buf_(static_cast<uint8_t*>(std::allocator<uint8_t>{}.allocate(capacity))),
  cap_(capacity),
  mask_(capacity - 1) {
    assert((cap_ & (cap_ - 1)) == 0);
  }


River::~River() {
  std::allocator<uint8_t>{}.deallocate(buf_, cap_);
}

std::pair<const uint8_t*, size_t> River::reserve_write() {
  const size_t w = write_.load(std::memory_order_relaxed);
  const size_t r = read_.load(std::memory_order_acquire);

  const size_t used = w - r;
  const size_t free = cap_ - used;

  const size_t wpos = w & mask_;
  const size_t to_end = cap_ - wpos;

  const size_t n = (free < to_end) ? free : to_end;
  return {buf_ + wpos, n};
}

void River::commit_write(size_t n) {
  write_.fetch_add(n, std::memory_order_release);
}

std::pair<const uint8_t*, size_t> River::reserve_read() {
  const size_t w = write_.load(std::memory_order_acquire);
  const size_t r = read_.load(std::memory_order_relaxed);

  const size_t available = w - r;
  const size_t rpos = r & mask_;
  const size_t to_end = cap_ - rpos;

  const size_t n = (available < to_end) ? available : to_end;
  return {buf_ + rpos, n};
}

void River::commit_read(size_t n) {
  read_.fetch_add(n, std::memory_order_release);
}

} // namespace Riverbed
