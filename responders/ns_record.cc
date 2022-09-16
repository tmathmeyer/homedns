
#include "responders.h"

namespace homedns {

PacketStatus::Or<DnsPacket> ReplyNSRecord(const DnsQuestion* question,
                                          DnsPacket&& response) {
  return PacketStatus::Codes::kInvalidRecordType;
}

}  // namespace homedns