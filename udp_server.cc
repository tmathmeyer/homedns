#include "udp_server.h"

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <cstring>

namespace homedns {

namespace {

void DoNotReply(Response, uint8_t*, size_t, struct sockaddr_in) {}

}  // namespace

std::unique_ptr<UDPServer> UDPServer::Create(uint16_t port) {
  int sockfd;
  if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
    perror("no socket");
    return nullptr;
  }
  struct sockaddr_in server_address;
  memset(&server_address, 0, sizeof(server_address));
  server_address.sin_family = AF_INET;
  server_address.sin_addr.s_addr = inet_addr("127.0.0.1");
  server_address.sin_port = htons(port);
  const sockaddr* addr = reinterpret_cast<sockaddr*>(&server_address);
  if (bind(sockfd, addr, sizeof(server_address)) < 0) {
    perror("bind failed");
    return nullptr;
  }
  return std::unique_ptr<UDPServer>(new UDPServer(sockfd));
}

UDPServer::~UDPServer() {
  close(socket_);
}

UDPServer::UDPServer(int socket)
    : socket_(socket), cb_(base::BindRepeating(&DoNotReply)) {}

int UDPServer::SendData(const uint8_t* data,
                        size_t len,
                        struct sockaddr_in client_addr) {
  return sendto(socket_, data, len, MSG_CONFIRM,
                reinterpret_cast<sockaddr*>(&client_addr), sizeof(client_addr));
}

void UDPServer::OnData(UDPServer::DataCB cb) {
  cb_ = std::move(cb);
}

void UDPServer::Start() {
  uint8_t buf[512];
  struct sockaddr_in client_addr;
  memset(&client_addr, 0, sizeof(client_addr));
  socklen_t client_len = sizeof(client_addr);
  sockaddr* addr = reinterpret_cast<sockaddr*>(&client_addr);
  size_t bytes = recvfrom(socket_, buf, 512, MSG_WAITALL, addr, &client_len);
  Response response{this, client_addr};
  cb_.Run(std::move(response), buf, bytes, client_addr);
}

Response::Response(UDPServer* server, struct sockaddr_in client_addr)
    : server_(server), client_addr_(client_addr) {}

int Response::SendData(const uint8_t* data, size_t len) {
  return server_->SendData(data, len, client_addr_);
}

int Response::SendData(std::vector<uint8_t> data) {
  return SendData(data.data(), data.size());
}

}  // namespace homedns