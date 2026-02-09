#include "Riverbed/river.hpp"

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>
#include <iostream>
#include <cstdint>

void run_simple_server(uint16_t port) {
  int listen_fd = ::socket(AF_INET, SOCK_STREAM, 0);
  if (listen_fd < 0) { std::perror("socket"); return; }

  int opt = 1;
  ::setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

  sockaddr_in addr;
  std::memset(&addr, 0, sizeof(addr));
  addr.sin_family = AF_INET;
  addr.sin_addr.s_addr = htonl(INADDR_ANY);
  addr.sin_port = htons(port);

  if (::bind(listen_fd, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) < 0) {
    std::perror("bind");
    ::close(listen_fd);
    return;
  }

  if (::listen(listen_fd, 1) < 0) {
    std::perror("listen");
    ::close(listen_fd);
    return;
  }

  int client_fd = ::accept(listen_fd, nullptr, nullptr);
  if (client_fd < 0) {
    std::perror("accept");
    ::close(listen_fd);
    return;
  }

  constexpr size_t BUF_SIZE = 64 * 1024;
  uint8_t buf[BUF_SIZE];

  while (true) {
    ssize_t nread = ::recv(client_fd, buf, BUF_SIZE, 0);
    if (nread < 0) { std::perror("recv"); break; }
    if (nread == 0) break;

    size_t total_sent = 0;
    while (total_sent < static_cast<size_t>(nread)) {
      ssize_t nsent = ::send(client_fd,
                             buf + total_sent,
                             static_cast<size_t>(nread) - total_sent,
                             0);
      if (nsent < 0) { std::perror("send"); goto done_simple; }
      total_sent += static_cast<size_t>(nsent);
    }
  }

done_simple:
  ::close(client_fd);
  ::close(listen_fd);
}

void run_river_server(uint16_t port) {
  int listen_fd = ::socket(AF_INET, SOCK_STREAM, 0);
  if (listen_fd < 0) { std::perror("socket"); return; }

  int opt = 1;
  ::setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

  sockaddr_in addr;
  std::memset(&addr, 0, sizeof(addr));
  addr.sin_family = AF_INET;
  addr.sin_addr.s_addr = htonl(INADDR_ANY);
  addr.sin_port = htons(port);
  addr.sin_addr.s_addr = htonl(INADDR_ANY);

  if (::bind(listen_fd, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) < 0) {
    std::perror("bind");
    ::close(listen_fd);
    return;
  }

  if (::listen(listen_fd, 1) < 0) {
    std::perror("listen");
    ::close(listen_fd);
    return;
  }

  int client_fd = ::accept(listen_fd, nullptr, nullptr);
  if (client_fd < 0) {
    std::perror("accept");
    ::close(listen_fd);
    return;
  }

  Riverbed::River river(1 << 16);

  while (true) {
    auto [dst, writable] = river.reserve_write();
    if (writable == 0) continue;

    ssize_t nread = ::recv(client_fd, const_cast<uint8_t*>(dst), writable, 0);
    if (nread < 0) { std::perror("recv"); break; }
    if (nread == 0) break;

    river.commit_write(static_cast<size_t>(nread));

    while (true) {
      auto [src, readable] = river.reserve_read();
      if (readable == 0) break;

      ssize_t nsent = ::send(client_fd, src, readable, 0);
      if (nsent < 0) { std::perror("send"); goto done_river; }

      river.commit_read(static_cast<size_t>(nsent));
      if (nsent < static_cast<ssize_t>(readable)) break;
    }
  }

done_river:
  ::close(client_fd);
  ::close(listen_fd);
}

int main(int argc, char** argv) {
  if (argc < 2) {
    std::cerr << "Usage: " << argv[0] << " simple|river\n";
    return 1;
  }

  const uint16_t port = 9000;
  std::string mode = argv[1];

  if (mode == "simple") {
    run_simple_server(port);
  } else if (mode == "river") {
    run_river_server(port);
  } else {
    std::cerr << "Unknown mode\n";
    return 1;
  }

  return 0;
}

