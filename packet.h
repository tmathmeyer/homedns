#pragma once

#include "bitstream.h"

namespace homedns {

struct DnsLabelSeq {
  DnsLabelSeq* Next;
  std::string Name;
};

struct DnsQuestion {
  DnsLabelSeq* LabelSequence;
  uint16_t Type;
  uint16_t Class;
};

struct DnsRecordPreamble {
  DnsLabelSeq* LabelSequence;
  uint16_t Type;
  uint16_t Class;
  uint32_t TTL;
  uint16_t Length;
};

struct DnsPacketHeader {
  uint16_t ID : 16; // ID
  uint16_t QR : 1;  // Question / Response
  uint16_t OP : 4;  // Operation Code
  uint16_t AA : 1;  // ?AuthoritativeAnswer
  uint16_t TC : 1;  // ?TruncatedMessage
  uint16_t RD : 1;  // ?RecursionDesired
  uint16_t RA : 1;  // ?RecursionAvailable
  uint16_t RZ : 3;  // Reserved
  uint16_t RC : 4;  // Response Code
  uint16_t QC : 16; // Question Count [DnsQuestion]
  uint16_t AC : 16; // Answer Count [DnsRecord]
  uint16_t NC : 16; // Authority Count [DnsRecord]
  uint16_t DC : 16; // Additional Count [DnsRecord]
} __attribute__((packed));

void Write(const DnsPacketHeader& header, WriteStream* bitstream);
void Read(DnsRecordPreamble* preamble, ReadStream* bitstream);
void Read(DnsPacketHeader* header, ReadStream* bitstream);







static_assert(sizeof(DnsPacketHeader) == 12);

}