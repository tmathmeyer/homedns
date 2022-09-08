#include "udp_server.h"

#include <netinet/in.h>
#include <sys/socket.h>
#include <cstring>

namespace homedns {

namespace {

void DoNotReply(UDPServer*, uint8_t*, size_t, struct sockaddr_in) {}

}

std::unique_ptr<UDPServer> UDPServer::Create(uint16_t port) {
  int sockfd;
  if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
    return nullptr;
  struct sockaddr_in server_address;
  memset(&server_address, 0, sizeof(server_address));
  server_address.sin_family = AF_INET;
  server_address.sin_addr.s_addr = INADDR_ANY;
  server_address.sin_port = htons(port);
  const sockaddr* addr = reinterpret_cast<sockaddr*>(&server_address);
  if (bind(sockfd, addr, sizeof(server_address)) < 0)
    return nullptr;
  return std::unique_ptr<UDPServer>(new UDPServer(sockfd));
}

UDPServer::~UDPServer() {
  close(socket_);
}

UDPServer::UDPServer(int socket)
    : socket_(socket), cb_(&DoNotReply) {}

int UDPServer::SendData(std::vector<uint8_t> data) {
  return SendData(data.data(), data.size());
}

int UDPServer::SendData(uint8_t* data, size_t len) {
  (void)data;
  (void)len;
  return -1;
}

void UDPServer::OnData(UDPServer::DataCB cb) {
  puts("OnData");
  cb_ = cb;
}

void UDPServer::Start() {
  puts("UDPServer::Start");
  uint8_t buf[512];
  struct sockaddr_in client_addr;
  memset(&client_addr, 0, sizeof(client_addr));
  socklen_t client_len = sizeof(client_addr);
  sockaddr* addr = reinterpret_cast<sockaddr*>(&client_addr);
  size_t bytes = recvfrom(socket_, buf, 512, MSG_WAITALL, addr, &client_len);
  cb_(this, buf, bytes, client_addr);
}

}  // namespace homedns