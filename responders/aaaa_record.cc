
#include "responders.h"

namespace homedns {

PacketStatus::Or<DnsPacket> ReplyAAAARecord(const DnsQuestion* question,
                                            DnsPacket&& response) {
  return PacketStatus::Codes::kInvalidRecordType;
}

}  // namespace homedns