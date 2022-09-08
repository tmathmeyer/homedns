
#include <arpa/inet.h>
#include <iostream>

#include "base/bind/bind.h"
#include "base/json/json_io.h"

#include "bitstream.h"
#include "packet.h"
#include "udp_server.h"

namespace homedns {

PacketStatus::Or<DnsPacket> RespondTo(const DnsQuestion* question,
                                      DnsPacket response) {
  return std::move(response);
}

void OnRequest(UDPServer* server,
               uint8_t* data,
               size_t len,
               struct sockaddr_in client) {
  // 50.35.80.74, 127.0.0.1
  uint32_t safe_addr = 0x4A502332;
  uint32_t home_addr = 0x0100007F;
  uint32_t cli_addr = client.sin_addr.s_addr;
  if (cli_addr != safe_addr && cli_addr != home_addr)
    return;

  auto m_packet = DnsPacket::Import(std::make_unique<ReadStream>(len, data));

  // If we can't parse
  if (!m_packet.has_value()) {
    std::move(m_packet).error().Print();
    return;
  }

  DnsPacket query = std::move(m_packet).value();
  DnsPacket response =
      DnsPacket::Create(query.GetPacketHeader().ID)
          .SetQuestionOrResponse(DnsPacket::PacketType::kResponse)
          .SetOpCode(0)
          .SetIsAuthoritative(1)
          .SetIsTruncated(0)
          .SetRecursionDesired(1)
          .SetRecursionAvailable(0)
          .SetResponseCode(0)
          .SetReserved(0);
  size_t q_count = query.GetNumQuestions();
  for (size_t q_index = 0; q_index < q_count; q_index++) {
    std::optional<const DnsQuestion*> q = query.GetQuestion(q_index);
    if (!q.has_value()) {
      perror("Tried to get a question with bounds check, but failed");
      std::cout << query.Render() << "\n";
      exit(1);
    }
    auto m_response = RespondTo(q.value(), std::move(response));
    if (m_response.has_value()) {
      response = std::move(m_response).value();
    } else {
      std::move(m_response).error().Print();
      return;
    }
  }

  auto ws = std::make_unique<WriteStream>(512);
  auto ext = response.Export(ws.get());
  if (!ext.is_ok()) {
    ext.Print();
    return;
  }
  auto rs = ws->Convert();
  server->SendData(rs->GetBuffer(), rs->Size());
}

}  // namespace homedns

int main() {
  auto server = homedns::UDPServer::Create(53);
  if (!server) {
    return 1;
  }
  server->OnData(base::BindRepeating(&homedns::OnRequest));
  server->Start();
}