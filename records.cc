#include "records.h"

#include "packet.h"

namespace homedns {

#define RETURN_ON_ERROR(expr)         \
  do {                                \
    auto st = (expr);                 \
    if (!st.is_ok())                  \
      return std::move(st).AddHere(); \
  } while (0)

#define CAUSE_ON_ERROR(expr)                                  \
  do {                                                        \
    auto st = (expr);                                         \
    if (!st.is_ok())                                          \
      return PacketStatus(PacketStatus::Codes::kParsingError) \
          .AddCause(std::move(st));                           \
  } while (0)

PacketStatus DnsARecord::Export(WriteStream* stream,
                                LabelManager* labels) const {
  (void)labels;
  for (int i = 0; i < 4; i++)
    CAUSE_ON_ERROR(stream->Write<8>(IP[i]));
  return base::OkStatus();
}

PacketStatus DnsARecord::Import(ReadStream* stream, LabelManager* labels) {
  (void)labels;
  for (int i = 0; i < 4; i++)
    CAUSE_ON_ERROR(stream->Next<8>(&IP[i]));
  return base::OkStatus();
}

base::json::Object DnsARecord::Render() const {
  std::stringstream stream;
  stream << (int)IP[0] << "." << (int)IP[1] << "." << (int)IP[2] << "."
         << (int)IP[3];
  std::map<std::string, base::json::JSON> result;
  result["IP"] = stream.str();
  return base::json::Object(std::move(result));
}

PacketStatus DnsNSRecord::Export(WriteStream* stream,
                                 LabelManager* labels) const {
  auto m_seq = labels->GetLabelSeq(label);
  if (m_seq.has_error())
    return std::move(m_seq).error();
  auto seq = std::move(m_seq).value();
  return labels->ExportLabelSeq(stream, seq.get());
}

PacketStatus DnsNSRecord::Import(ReadStream* stream, LabelManager* labels) {
  auto m_seq = labels->ImportLabelSequence(stream);
  if (m_seq.has_error())
    return std::move(m_seq).error();
  label = std::move(m_seq).value()->Render();
  return base::OkStatus();
}

base::json::Object DnsNSRecord::Render() const {
  std::map<std::string, base::json::JSON> result;
  result["Label"] = label;
  return base::json::Object(std::move(result));
}

PacketStatus DnsCNAMERecord::Export(WriteStream* stream,
                                    LabelManager* labels) const {
  auto m_seq = labels->GetLabelSeq(label);
  if (m_seq.has_error())
    return std::move(m_seq).error();
  auto seq = std::move(m_seq).value();
  return labels->ExportLabelSeq(stream, seq.get());
}

PacketStatus DnsCNAMERecord::Import(ReadStream* stream, LabelManager* labels) {
  auto m_seq = labels->ImportLabelSequence(stream);
  if (m_seq.has_error())
    return std::move(m_seq).error();
  label = std::move(m_seq).value()->Render();
  return base::OkStatus();
}

base::json::Object DnsCNAMERecord::Render() const {
  std::map<std::string, base::json::JSON> result;
  result["Name"] = label;
  return base::json::Object(std::move(result));
}

PacketStatus DnsAAAARecord::Export(WriteStream* stream,
                                   LabelManager* labels) const {
  (void)labels;
  for (int i = 0; i < 16; i++)
    CAUSE_ON_ERROR(stream->Write<8>(IP[i]));
  return base::OkStatus();
}

PacketStatus DnsAAAARecord::Import(ReadStream* stream, LabelManager* labels) {
  (void)labels;
  for (int i = 0; i < 16; i++)
    CAUSE_ON_ERROR(stream->Next<8>(&IP[i]));
  return base::OkStatus();
}

base::json::Object DnsAAAARecord::Render() const {
  std::stringstream stream;
  stream << std::hex << (int)IP[0] << (int)IP[1]
         << ":"                                // TODO improve this serializer
         << (int)IP[2] << (int)IP[3] << ":"    // so that it can do the ::
         << (int)IP[4] << (int)IP[5] << ":"    // stuff when there are 0000
         << (int)IP[6] << (int)IP[7] << ":"    // entries in one of the 16 bit
         << (int)IP[8] << (int)IP[9] << ":"    // chunks. Also this comment
         << (int)IP[10] << (int)IP[11] << ":"  // exists to keep the nice
         << (int)IP[12] << (int)IP[13] << ":"  // alignment. lol.
         << (int)IP[14] << (int)IP[15];
  std::map<std::string, base::json::JSON> result;
  result["IP"] = stream.str();
  return base::json::Object(std::move(result));
}

PacketStatus DnsMXRecord::Export(WriteStream* stream,
                                 LabelManager* labels) const {
  CAUSE_ON_ERROR(stream->Write<8>(priority[0]));
  CAUSE_ON_ERROR(stream->Write<8>(priority[1]));
  auto m_seq = labels->GetLabelSeq(label);
  if (m_seq.has_error())
    return std::move(m_seq).error();
  auto seq = std::move(m_seq).value();
  return labels->ExportLabelSeq(stream, seq.get());
}

PacketStatus DnsMXRecord::Import(ReadStream* stream, LabelManager* labels) {
  CAUSE_ON_ERROR(stream->Next<8>(&priority[0]));
  CAUSE_ON_ERROR(stream->Next<8>(&priority[1]));
  auto m_seq = labels->ImportLabelSequence(stream);
  if (m_seq.has_error())
    return std::move(m_seq).error();
  label = std::move(m_seq).value()->Render();
  return base::OkStatus();
}

base::json::Object DnsMXRecord::Render() const {
  std::map<std::string, base::json::JSON> result;
  result["Priority"] = "TODO: serialize";
  result["Label"] = label;
  return base::json::Object(std::move(result));
}

PacketStatus DnsUnknownRecord::Export(WriteStream* stream,
                                      LabelManager* labels) const {
  (void)labels;
  for (const uint8_t byte : data)
    CAUSE_ON_ERROR(stream->Write<8>(byte));
  return base::OkStatus();
}

}  // namespace homedns