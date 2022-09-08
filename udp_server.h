#pragma once

#include <vector>
#include <netinet/in.h>

#include "base/bind/bind.h"

namespace homedns {

class UDPServer {
 public:
  using DataCB = base::RepeatingCallback<
      void(UDPServer*, uint8_t*, size_t, struct sockaddr_in)>;
  static std::unique_ptr<UDPServer> Create(uint16_t port);

  ~UDPServer();
  int SendData(std::vector<uint8_t> data);
  int SendData(const uint8_t* data, size_t len);
  void OnData(DataCB cb);

  void Start();

 private:
  UDPServer(int socket);

  int socket_;
  DataCB cb_;
};

}  // namespace homedns