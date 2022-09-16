
#include "responders.h"

namespace homedns {

PacketStatus::Or<DnsPacket> ReplyLOCRecord(const DnsQuestion* question,
                                           DnsPacket&& response) {
  return PacketStatus::Codes::kInvalidRecordType;
}

}  // namespace homedns