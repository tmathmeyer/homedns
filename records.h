#pragma once

#include <variant>

#include "base/json/json.h"

#include "bitstream.h"
#include "labels.h"
#include "status.h"

namespace homedns {

class DnsPacket;

struct DnsARecord {
  static constexpr uint16_t TYPE = 1;
  PacketStatus Import(ReadStream* stream, LabelManager* labels);
  PacketStatus Export(WriteStream* stream, LabelManager* labels) const;
  base::json::Object Render() const;
  uint8_t IP[4];
};

struct DnsNSRecord {
  static constexpr uint16_t TYPE = 2;
  PacketStatus Import(ReadStream* stream, LabelManager* labels);
  PacketStatus Export(WriteStream* stream, LabelManager* labels) const;
  base::json::Object Render() const;

  std::string label;
};

struct DnsCNAMERecord {
  static constexpr uint16_t TYPE = 5;
  PacketStatus Import(ReadStream* stream, LabelManager* labels);
  PacketStatus Export(WriteStream* stream, LabelManager* labels) const;
  base::json::Object Render() const;

  std::string label;
};

struct DnsMXRecord {
  static constexpr uint16_t TYPE = 15;
  PacketStatus Import(ReadStream* stream, LabelManager* labels);
  PacketStatus Export(WriteStream* stream, LabelManager* labels) const;
  base::json::Object Render() const;

  uint8_t priority[2];
  std::string label;
};

struct DnsAAAARecord {
  static constexpr uint16_t TYPE = 28;
  PacketStatus Import(ReadStream* stream, LabelManager* labels);
  PacketStatus Export(WriteStream* stream, LabelManager* labels) const;
  base::json::Object Render() const;

  uint8_t IP[16];
};

struct DnsLOCRecord {
  static constexpr uint16_t TYPE = 29;
  PacketStatus Import(ReadStream* stream, LabelManager* labels);
  PacketStatus Export(WriteStream* stream, LabelManager* labels) const;
  base::json::Object Render() const;

  // TODO: what goes here?
  int foo;
};

struct DnsRPRecord {
  static constexpr uint16_t TYPE = 17;
  PacketStatus Import(ReadStream* stream, LabelManager* labels);
  PacketStatus Export(WriteStream* stream, LabelManager* labels) const;
  base::json::Object Render() const;

  std::string email;
};

struct DnsSOARecord {
  static constexpr uint16_t TYPE = 6;
  PacketStatus Import(ReadStream* stream, LabelManager* labels);
  PacketStatus Export(WriteStream* stream, LabelManager* labels) const;
  base::json::Object Render() const;

  std::string email;
};

struct DnsTXTRecord {
  static constexpr uint16_t TYPE = 16;
  PacketStatus Import(ReadStream* stream, LabelManager* labels);
  PacketStatus Export(WriteStream* stream, LabelManager* labels) const;
  base::json::Object Render() const;

  std::string email;
};

struct DnsUnknownRecord {
  std::vector<uint8_t> data;
  PacketStatus Export(WriteStream* stream, LabelManager* labels) const;
  base::json::Object Render() const;
};

using DnsRecord = std::variant<DnsARecord,
                               DnsNSRecord,
                               DnsCNAMERecord,
                               DnsMXRecord,
                               DnsAAAARecord,
                               DnsUnknownRecord>;

}  // namespace homedns