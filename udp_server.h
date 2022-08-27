#pragma once

namespace homends {

class UDPServer {
 public:
  ~UDPServer();
  UDPServer(uint16_t port);
  virtual std::vector<uint8_t> OnData(std::vector<uint8_t> data);
};

}