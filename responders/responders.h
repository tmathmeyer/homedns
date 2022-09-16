#pragma once

#include "../packet.h"
#include "../records.h"
#include "../status.h"

namespace homedns {

PacketStatus::Or<DnsPacket> ReplyARecord(const DnsQuestion* question,
                                         DnsPacket&& response);
PacketStatus::Or<DnsPacket> ReplyNSRecord(const DnsQuestion* question,
                                          DnsPacket&& response);
PacketStatus::Or<DnsPacket> ReplyCNAMERecord(const DnsQuestion* question,
                                             DnsPacket&& response);
PacketStatus::Or<DnsPacket> ReplyMXRecord(const DnsQuestion* question,
                                          DnsPacket&& response);
PacketStatus::Or<DnsPacket> ReplyAAAARecord(const DnsQuestion* question,
                                            DnsPacket&& response);
PacketStatus::Or<DnsPacket> ReplyLOCRecord(const DnsQuestion* question,
                                           DnsPacket&& response);
PacketStatus::Or<DnsPacket> ReplyRPRecord(const DnsQuestion* question,
                                          DnsPacket&& response);
PacketStatus::Or<DnsPacket> ReplySOARecord(const DnsQuestion* question,
                                           DnsPacket&& response);
PacketStatus::Or<DnsPacket> ReplyTXTRecord(const DnsQuestion* question,
                                           DnsPacket&& response);
PacketStatus::Or<DnsPacket> ReplyUnknownRecord(const DnsQuestion* question,
                                               DnsPacket&& response);

}  // namespace homedns