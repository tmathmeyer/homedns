
#include "responders.h"

namespace homedns {

PacketStatus::Or<DnsPacket> ReplyTXTRecord(const DnsQuestion* question,
                                           DnsPacket&& response) {
  return PacketStatus::Codes::kInvalidRecordType;
}

}  // namespace homedns