#include "Riverbed/river.hpp"

#include <cassert>
#include <chrono>
#include <condition_variable>
#include <iostream>
#include <mutex>
#include <queue>
#include <thread>

int main() {
  constexpr int N = 1'000'000;
  Riverbed::RingBuffer<int> rb(1024);

  auto start_rb = std::chrono::high_resolution_clock::now();

  std::thread producer_rb([&] {
    for (int i = 0; i < N; ++i) {
      while (!rb.push(i)) {
      }
    }
  });

  std::thread consumer_rb([&] {
    int expected = 0;
    while (expected < N) {
      int value;
      if (rb.pop(value)) {
        assert(value == expected);
        ++expected;
      }
    }
  });

  producer_rb.join();
  consumer_rb.join();

  auto end_rb = std::chrono::high_resolution_clock::now();
  auto ns_rb = std::chrono::duration_cast<std::chrono::nanoseconds>(end_rb - start_rb).count();
  double sec_rb = static_cast<double>(ns_rb) / 1e9;
  double thr_rb = static_cast<double>(N) / sec_rb;

  std::cout << "RingBuffer time: " << ns_rb << " ns\n";
  std::cout << "RingBuffer throughput: " << thr_rb << " items/sec\n";

  std::queue<int> q;
  std::mutex m;
  std::condition_variable cv;
  bool done = false;

  auto start_q = std::chrono::high_resolution_clock::now();

  std::thread producer_q([&] {
    for (int i = 0; i < N; ++i) {
      {
        std::lock_guard<std::mutex> lk(m);
        q.push(i);
      }
      cv.notify_one();
    }
    {
      std::lock_guard<std::mutex> lk(m);
      done = true;
    }
    cv.notify_one();
  });

  std::thread consumer_q([&] {
    int expected = 0;
    for (;;) {
      std::unique_lock<std::mutex> lk(m);
      cv.wait(lk, [&] { return !q.empty() || done; });
      while (!q.empty()) {
        int value = q.front();
        q.pop();
        lk.unlock();
        assert(value == expected);
        ++expected;
        lk.lock();
      }
      if (done && q.empty()) break;
    }
    assert(expected == N);
  });

  producer_q.join();
  consumer_q.join();

  auto end_q = std::chrono::high_resolution_clock::now();
  auto ns_q = std::chrono::duration_cast<std::chrono::nanoseconds>(end_q - start_q).count();
  double sec_q = static_cast<double>(ns_q) / 1e9;
  double thr_q = static_cast<double>(N) / sec_q;

  std::cout << "Mutex+queue time: " << ns_q << " ns\n";
  std::cout << "Mutex+queue throughput: " << thr_q << " items/sec\n";
}

