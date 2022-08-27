#pragma once

#include <cstdint>
#include <cstdlib>

#include "bitstream.h"
#include "status.h"

namespace homedns {

using LongForm = std::string;
using ShortForm = std::string;

struct Segment {
  ShortForm segment;
  LongForm longform;
  Segment* next;
};

struct DnsLabelSeq {
  Segment* value;
  std::string Render() { return value ? value->longform : "ERROR"; }
};

class LabelManager {
 private:
  std::map<LongForm, std::unique_ptr<Segment>> segments_;
  std::map<Segment*, uint16_t> segment_write_positions_;

  Segment* ExpandLongForm(LongForm input);
  PacketStatus::Or<Segment*> Import(ReadStream* stream);
  PacketStatus::Or<Segment*> ImportNonDestructive(const ReadStream* stream,
                                                  uint16_t address);

 public:
  void ResetWritePositions();
  PacketStatus::Or<std::unique_ptr<DnsLabelSeq>> GetLabelSeq(LongForm);

  PacketStatus ExportLabelSeq(WriteStream* stream, DnsLabelSeq* seq);
  PacketStatus::Or<std::unique_ptr<DnsLabelSeq>> ImportLabelSequence(
      ReadStream* stream);
};

}  // namespace homedns