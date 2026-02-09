#pragma once 


#include <atomic>   
#include <cstddef>   
#include <cstdint>   
#include <cassert>   
#include <utility>     

namespace Riverbed {
  
class River {
public:
  explicit River(size_t capacity);

  ~River();

  std::pair<const uint8_t*, size_t> reserve_write();

  void commit_write(size_t n);

  std::pair<const uint8_t*, size_t> reserve_read();

  void commit_read(size_t n);

  size_t capacity_bytes() const { return cap_; }

private:
  uint8_t* buf_;
  size_t cap_;
  size_t mask_;

  alignas(64) std::atomic<size_t> write_{0};
  alignas(64) std::atomic<size_t> read_{0};
}; 

} // namespace Riverbed
