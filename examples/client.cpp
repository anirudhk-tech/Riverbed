#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <chrono>
#include <cstring>
#include <iostream>
#include <vector>
#include <cstdint>

int main() {
  const char* host = "127.0.0.1";
  const uint16_t port = 9000;
  const size_t message_size = 1024;
  const size_t iterations   = 100000;

  int fd = ::socket(AF_INET, SOCK_STREAM, 0);
  if (fd < 0) {
    std::perror("socket");
    return 1;
  }

  sockaddr_in addr;
  std::memset(&addr, 0, sizeof(addr));
  addr.sin_family = AF_INET;
  addr.sin_port = htons(port);
  if (::inet_pton(AF_INET, host, &addr.sin_addr) <= 0) {
    std::perror("inet_pton");
    ::close(fd);
    return 1;
  }

  if (::connect(fd, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) < 0) {
    std::perror("connect");
    ::close(fd);
    return 1;
  }

  std::vector<uint8_t> send_buf(message_size);
  std::vector<uint8_t> recv_buf(message_size);

  for (size_t i = 0; i < message_size; ++i) {
    send_buf[i] = static_cast<uint8_t>(i & 0xFF);
  }

  auto start = std::chrono::steady_clock::now();

  for (size_t iter = 0; iter < iterations; ++iter) {
    size_t total_sent = 0;
    while (total_sent < message_size) {
      ssize_t nsent = ::send(fd,
                             send_buf.data() + total_sent,
                             message_size - total_sent,
                             0);
      if (nsent < 0) {
        std::perror("send");
        ::close(fd);
        return 1;
      }
      total_sent += static_cast<size_t>(nsent);
    }

    size_t total_recv = 0;
    while (total_recv < message_size) {
      ssize_t nread = ::recv(fd,
                             recv_buf.data() + total_recv,
                             message_size - total_recv,
                             0);
      if (nread < 0) {
        std::perror("recv");
        ::close(fd);
        return 1;
      }
      if (nread == 0) {
        std::cerr << "Server closed connection early\n";
        ::close(fd);
        return 1;
      }
      total_recv += static_cast<size_t>(nread);
    }
  }

  auto end = std::chrono::steady_clock::now();

  std::chrono::duration<double> elapsed = end - start;
  double seconds = elapsed.count();

  const double total_bytes =
      static_cast<double>(iterations) *
      static_cast<double>(message_size) * 2.0;

  double mbps = (total_bytes / (1024.0 * 1024.0)) / seconds;

  std::cout << "Iterations:    " << iterations << "\n";
  std::cout << "Message size:  " << message_size << " bytes\n";
  std::cout << "Total bytes:   " << total_bytes << " bytes\n";
  std::cout << "Elapsed time:  " << seconds << " s\n";
  std::cout << "Throughput:    " << mbps << " MB/s\n";

  ::close(fd);
  return 0;
}

