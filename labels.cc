#include "labels.h"

#include <cstring>

#define CAUSE_ON_ERROR(expr)                                  \
  do {                                                        \
    auto st = (expr);                                         \
    if (!st.is_ok())                                          \
      return PacketStatus(PacketStatus::Codes::kParsingError) \
          .AddCause(std::move(st));                           \
  } while (0)

namespace homedns {

Segment* LabelManager::ExpandLongForm(LongForm input) {
  if (input.length() == 0)
    return nullptr;

  if (segments_.find(input) != segments_.end())
    return segments_[input].get();

  LongForm longform = input;
  size_t dotpos = input.find(".");
  if (dotpos == std::string::npos) {
    // It doesn't exist already, and has no "." in it, so create it.
    segments_[longform] = std::make_unique<Segment>(input, longform, nullptr);
  } else {
    ShortForm segment = input.substr(0, dotpos);
    input.erase(0, dotpos + 1);
    Segment* next = ExpandLongForm(input);
    if (next == nullptr)
      return nullptr;
    segments_[longform] = std::make_unique<Segment>(segment, longform, next);
  }
  return segments_[longform].get();
}

void LabelManager::ResetWritePositions() {
  segment_write_positions_.clear();
}

PacketStatus::Or<std::unique_ptr<DnsLabelSeq>> LabelManager::GetLabelSeq(
    LongForm input) {
  Segment* segment = ExpandLongForm(input);
  if (segment == nullptr)
    return PacketStatus::Codes::kParsingError;
  return std::make_unique<DnsLabelSeq>(segment);
}

PacketStatus LabelManager::ExportLabelSeq(WriteStream* stream,
                                          DnsLabelSeq* seq) {
  Segment* seg = seq->value;
  while (seg) {
    if (segment_write_positions_.find(seg) != segment_write_positions_.end()) {
      uint16_t position = segment_write_positions_[seg];
      CAUSE_ON_ERROR(stream->Write<16>(position | 0xC000));
      return base::OkStatus();
    }
    segment_write_positions_[seg] = stream->CurrentByte();
    CAUSE_ON_ERROR(stream->Write<8>(seg->segment.length()));
    for (size_t i = 0; i < seg->segment.length(); i++)
      CAUSE_ON_ERROR(stream->Write<8>(seg->segment.c_str()[i]));
    seg = seg->next;
  }
  CAUSE_ON_ERROR(stream->Write<8>(0));
  return base::OkStatus();
}

PacketStatus::Or<std::unique_ptr<DnsLabelSeq>>
LabelManager::ImportLabelSequence(ReadStream* stream) {
  auto m_segment = Import(stream);
  if (m_segment.has_error())
    return std::move(m_segment).error().AddHere();
  return std::make_unique<DnsLabelSeq>(std::move(m_segment).value());
}

PacketStatus::Or<Segment*> LabelManager::Import(ReadStream* stream) {
  uint8_t length;
  CAUSE_ON_ERROR(stream->Next<8>(&length));
  if (length == 0) {
    return nullptr;  // base case, NOT an error!
  } else if ((length & 0xC0) == 0xC0) {
    // Consume the second offset byte and continue to read the labels at the new
    // location without consuming new bytes.
    uint16_t location;
    CAUSE_ON_ERROR(stream->Next<8>(&location));
    location |= ((length ^ 0xC0) << 8);
    return ImportNonDestructive(stream, location);
  } else {
    std::stringstream token;
    while (length-- > 0) {
      uint8_t byte;
      CAUSE_ON_ERROR(stream->Next<8>(&byte));
      token << byte;
    }
    PacketStatus::Or<Segment*> rest = Import(stream);
    if (!rest.has_value())
      return std::move(rest).error().AddHere();
    Segment* next = std::move(rest).value();
    LongForm total = next ? (token.str() + "." + next->longform) : token.str();
    if (segments_.find(total) == segments_.end())
      segments_[total] = std::make_unique<Segment>(token.str(), total, next);
    return segments_[total].get();
  }
}

PacketStatus::Or<Segment*> LabelManager::ImportNonDestructive(
    const ReadStream* stream,
    uint16_t address) {
  uint8_t length;
  CAUSE_ON_ERROR(stream->Read<8>(&length, address));
  if (length == 0) {
    return nullptr;  // base case, NOT an error!
  } else if ((length & 0xC0) == 0xC0) {
    // Consume the second offset byte and continue to read the labels at the new
    // location without consuming new bytes.
    uint16_t location;
    address++;
    CAUSE_ON_ERROR(stream->Read<8>(&location, address));
    location |= ((length ^ 0xC0) << 8);
    return ImportNonDestructive(stream, location);
  } else {
    std::stringstream token;
    while (length-- > 0) {
      address++;
      uint8_t byte;
      CAUSE_ON_ERROR(stream->Read<8>(&byte, address));
      token << byte;
    }
    PacketStatus::Or<Segment*> rest = ImportNonDestructive(stream, address + 1);
    if (!rest.has_value())
      return std::move(rest).error().AddHere();
    Segment* next = std::move(rest).value();
    LongForm total = next ? (token.str() + "." + next->longform) : token.str();

    if (segments_.find(total) == segments_.end())
      segments_[total] = std::make_unique<Segment>(token.str(), total, next);
    return segments_[total].get();
  }
}

}  // namespace homedns