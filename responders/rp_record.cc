
#include "responders.h"

namespace homedns {

PacketStatus::Or<DnsPacket> ReplyRPRecord(const DnsQuestion* question,
                                          DnsPacket&& response) {
  return PacketStatus::Codes::kInvalidRecordType;
}

}  // namespace homedns