#pragma once

#include <netinet/in.h>
#include <vector>

#include "base/bind/bind.h"

namespace homedns {

class UDPServer;

class Response {
 public:
  Response(UDPServer* server, struct sockaddr_in client_addr);
  int SendData(std::vector<uint8_t> data);
  int SendData(const uint8_t* data, size_t len);

 private:
  UDPServer* server_;
  struct sockaddr_in client_addr_;
};

class UDPServer {
 public:
  using DataCB = base::RepeatingCallback<
      void(Response, uint8_t*, size_t, struct sockaddr_in)>;
  static std::unique_ptr<UDPServer> Create(uint16_t port);

  ~UDPServer();
  int SendData(const uint8_t* data, size_t len, struct sockaddr_in client_addr);
  void OnData(DataCB cb);

  void Start();

 private:
  UDPServer(int socket);

  int socket_;
  DataCB cb_;
};

}  // namespace homedns