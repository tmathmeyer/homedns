
#include <iostream>

#include "base/bind/bind.h"
#include "base/json/json_io.h"

#include "udp_server.h"
#include "packet.h"
#include "bitstream.h"

void OnRequest(homedns::UDPServer* server,
               uint8_t* data,
               size_t len,
               struct sockaddr_in client) {
  auto m_packet = homedns::DnsPacket::Import(
      std::make_unique<homedns::ReadStream>(len, data));

  if (!m_packet.has_value()) {
    std::cout << std::dec;
    std::move(m_packet).error().Print();
    exit(1);
  }

  homedns::DnsPacket packet = std::move(m_packet).value();
  std::cout << packet.Render() << "\n";
}

int main() {
  puts("create");
  auto server = homedns::UDPServer::Create(53);
  puts("set cb");
  server->OnData(&OnRequest);
  puts("start");
  server->Start();
}