
#include "responders.h"

namespace homedns {

PacketStatus::Or<DnsPacket> ReplySOARecord(const DnsQuestion* question,
                                           DnsPacket&& response) {
  return PacketStatus::Codes::kInvalidRecordType;
}

}  // namespace homedns