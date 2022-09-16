
#include <arpa/inet.h>
#include <iostream>

#include "base/bind/bind.h"
#include "base/json/json_io.h"

#include "bitstream.h"
#include "packet.h"
#include "udp_server.h"

#include "responders/responders.h"

namespace homedns {

PacketStatus::Or<DnsPacket> RespondTo(const DnsQuestion* question,
                                      DnsPacket response) {
  response = std::move(response).AddQuestion(*question).Unwrap();

  switch(question->Type) {
    case DnsARecord::TYPE:
      return ReplyARecord(question, std::move(response));
    case DnsNSRecord::TYPE:
      return ReplyNSRecord(question, std::move(response));
    case DnsCNAMERecord::TYPE:
      return ReplyCNAMERecord(question, std::move(response));
    case DnsMXRecord::TYPE:
      return ReplyMXRecord(question, std::move(response));
    case DnsAAAARecord::TYPE:
      return ReplyAAAARecord(question, std::move(response));
    case DnsLOCRecord::TYPE:
      return ReplyLOCRecord(question, std::move(response));
    case DnsRPRecord::TYPE:
      return ReplyRPRecord(question, std::move(response));
    case DnsSOARecord::TYPE:
      return ReplySOARecord(question, std::move(response));
    case DnsTXTRecord::TYPE:
      return ReplyTXTRecord(question, std::move(response));
    default:
      return PacketStatus::Codes::kInvalidRecordType;
  }
}

void OnRequest(Response write_out,
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

  // If we can't parse the incoming packet, do _not_ write back. It's probably
  // some kind of nasty hacking attack, and we might as well just mess with the
  // sender.
  if (!m_packet.has_value()) {
    std::move(m_packet).error().Print();
    return;
  }

  // Build the default response packet
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
      // TODO: figure out how we reply here, since this was our failure to add
      // a response.
      return;
    }
  }

  auto ws = std::make_unique<WriteStream>(512);
  auto ext = response.Export(ws.get());
  if (!ext.is_ok()) {
    ext.Print();
    // TODO: figure out how to reply here.
    return;
  }
  auto rs = ws->Convert();
  write_out.SendData(rs->GetBuffer(), rs->Size());
}

}  // namespace homedns


int main() {
  auto server = homedns::UDPServer::Create(5300);
  if (!server) {
    return 1;
  }
  server->OnData(base::BindRepeating(&homedns::OnRequest));
  server->Start();
}