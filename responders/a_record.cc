
#include "responders.h"

namespace homedns {

PacketStatus::Or<DnsPacket> ReplyARecord(const DnsQuestion* question,
                                         DnsPacket&& response) {
  return std::move(response).AddRecord<homedns::DnsPacket::RecordType::kAnswer>(
      question->LabelSequence->Render(), question->Class,
      /*TTL = */ 100, DnsARecord({192, 168, 1, 1}));
}

}  // namespace homedns