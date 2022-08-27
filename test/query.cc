#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include "base/json/json_io.h"
#include "homedns/bitstream.h"
#include "homedns/packet.h"
#include "homedns/records.h"

std::unique_ptr<homedns::ReadStream> BuildQuery() {
  uint16_t packet_id = 0x12b5;
  homedns::DnsPacket packet =
      homedns::DnsPacket::Create(packet_id)
          .SetQuestionOrResponse(homedns::DnsPacket::PacketType::kQuestion)
          .SetOpCode(0)
          .SetIsAuthoritative(0)
          .SetIsTruncated(0)
          .SetRecursionDesired(1)
          .SetRecursionAvailable(0)
          .SetResponseCode(0)
          .SetReserved(0)
          .AddQuestion("www.google.com", homedns::DnsARecord::TYPE, 0x01)
          .Unwrap();

  std::cout << "Request Packet:\n" << packet.Render() << "\n";

  auto ws = std::make_unique<homedns::WriteStream>(512);
  auto ext = packet.Export(ws.get());
  if (!ext.is_ok()) {
    ext.Print();
    return nullptr;
  }

  return ws->Convert();
}

#define DNS_SIZE 512

int main() {
  int sockfd;
  char buffer[DNS_SIZE];
  struct sockaddr_in server_addr;

  if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
    puts("cant make socket");
    exit(1);
  }

  memset(&server_addr, 0, sizeof(struct sockaddr_in));
  server_addr.sin_family = AF_INET;
  server_addr.sin_port = htons(53);
  server_addr.sin_addr.s_addr = inet_addr("8.8.8.8");
  server_addr.sin_addr.s_addr = inet_addr("216.239.34.10");

  auto rs = BuildQuery();
  int result = sendto(sockfd, rs->GetBuffer(), rs->Size(), MSG_CONFIRM,
                      (const struct sockaddr *)&server_addr, sizeof(server_addr));
  puts("Sent!");

  socklen_t wb_len;
  int rec = recvfrom(sockfd, static_cast<char*>(buffer), DNS_SIZE,
    MSG_WAITALL, (struct sockaddr *)&server_addr, &wb_len);

  puts("Received");

  auto m_packet = homedns::DnsPacket::Import(
      std::make_unique<homedns::ReadStream>(rec, buffer));

  if (!m_packet.has_value()) {
    std::cout << std::dec;
    std::move(m_packet).error().Print();
    exit(1);
  }

  homedns::DnsPacket packet = std::move(m_packet).value();
  std::cout << packet.Render() << "\n";
}