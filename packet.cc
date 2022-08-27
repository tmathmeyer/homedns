#include "packet.h"

#include <sstream>

#include "base/json/json_io.h"

namespace homedns {

namespace {

template <size_t S>
struct uint_impl;

template <>
struct uint_impl<0> {
  using type = uint8_t;
};

template <>
struct uint_impl<1> {
  using type = uint16_t;
};

template <>
struct uint_impl<2> {
  using type = uint32_t;
};

template <>
struct uint_impl<3> {
  using type = uint_fast32_t;
};

template <size_t S>
struct uint_s {
  using type = typename uint_impl<(S > 8) + (S > 16) + (S > 32)>::type;
};

}  // namespace

#define ASSIGN_OR_ERROR(ato, expr)               \
  do {                                           \
    auto maybe = (expr);                         \
    if (maybe.has_error())                       \
      return std::move(maybe).error().AddHere(); \
    ato = std::move(maybe).value();              \
  } while (0)

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

#define READBITFIELD(bits, assign, stream)                        \
  do {                                                            \
    uint_s<bits>::type value;                                     \
    auto st = stream->Next<bits>(&value);                         \
    if (st.is_ok()) {                                             \
      assign = value;                                             \
    } else {                                                      \
      return {PacketStatus::Codes::kParsingError, std::move(st)}; \
    }                                                             \
  } while (0)

PacketStatus DnsPacketHeader::Import(DnsPacketHeader* header,
                                     ReadStream* stream) {
  READBITFIELD(16, header->ID, stream);
  READBITFIELD(1, header->QR, stream);
  READBITFIELD(4, header->OP, stream);
  READBITFIELD(1, header->AA, stream);
  READBITFIELD(1, header->TC, stream);
  READBITFIELD(1, header->RD, stream);
  READBITFIELD(1, header->RA, stream);
  READBITFIELD(3, header->RZ, stream);
  READBITFIELD(4, header->RC, stream);
  READBITFIELD(16, header->QC, stream);
  READBITFIELD(16, header->AC, stream);
  READBITFIELD(16, header->NC, stream);
  READBITFIELD(16, header->DC, stream);
  return base::OkStatus();
}

DnsPacket::DnsPacket(uint16_t ID) {
  header_ = std::make_unique<DnsPacketHeader>(
      /*.ID = */ ID,
      /*.QR = */ 0,
      /*.OP = */ 0,
      /*.AA = */ 0,
      /*.TC = */ 0,
      /*.RD = */ 0,
      /*.RA = */ 0,
      /*.RZ = */ 0,
      /*.RC = */ 0,
      /*.QC = */ 0,
      /*.AC = */ 0,
      /*.NC = */ 0,
      /*.DC = */ 0);
  label_manager_ = std::make_unique<LabelManager>();
}

DnsPacket DnsPacket::Create(uint16_t ID) {
  return DnsPacket{ID};
}

namespace _exporting {

PacketStatus ExportHeader(WriteStream* stream, const DnsPacketHeader& header) {
  CAUSE_ON_ERROR(stream->Write<16>(header.ID));
  CAUSE_ON_ERROR(stream->Write<1>(header.QR));
  CAUSE_ON_ERROR(stream->Write<4>(header.OP));
  CAUSE_ON_ERROR(stream->Write<1>(header.AA));
  CAUSE_ON_ERROR(stream->Write<1>(header.TC));
  CAUSE_ON_ERROR(stream->Write<1>(header.RD));
  CAUSE_ON_ERROR(stream->Write<1>(header.RA));
  CAUSE_ON_ERROR(stream->Write<3>(header.RZ));
  CAUSE_ON_ERROR(stream->Write<4>(header.RC));
  CAUSE_ON_ERROR(stream->Write<16>(header.QC));
  CAUSE_ON_ERROR(stream->Write<16>(header.AC));
  CAUSE_ON_ERROR(stream->Write<16>(header.NC));
  CAUSE_ON_ERROR(stream->Write<16>(header.DC));
  return base::OkStatus();
}

PacketStatus ExportQuestion(WriteStream* stream,
                            const DnsQuestion& question,
                            LabelManager* labels) {
  RETURN_ON_ERROR(labels->ExportLabelSeq(stream, question.LabelSequence.get()));
  CAUSE_ON_ERROR(stream->Write<16>(question.Type));
  CAUSE_ON_ERROR(stream->Write<16>(question.Class));
  return base::OkStatus();
}

PacketStatus ExportQuestions(WriteStream* stream,
                             const std::vector<DnsQuestion>& questions,
                             LabelManager* labels) {
  for (const auto& question : questions)
    RETURN_ON_ERROR(ExportQuestion(stream, question, labels));
  return base::OkStatus();
}

PacketStatus ExportRecord(WriteStream* stream,
                          const PreambleAndRecord& record,
                          LabelManager* labels) {
  RETURN_ON_ERROR(
      labels->ExportLabelSeq(stream, std::get<0>(record).LabelSequence.get()));
  CAUSE_ON_ERROR(stream->Write<16>(std::get<0>(record).Type));
  CAUSE_ON_ERROR(stream->Write<16>(std::get<0>(record).Class));
  CAUSE_ON_ERROR(stream->Write<32>(std::get<0>(record).TTL));
  uint16_t length_location = stream->CurrentByte();
  CAUSE_ON_ERROR(stream->Write<16>(0));

#define EXPORT(TYPENAME)                                                 \
  case TYPENAME::TYPE: {                                                 \
    RETURN_ON_ERROR(                                                     \
        std::get<TYPENAME>(std::get<1>(record)).Export(stream, labels)); \
    break;                                                               \
  }
  switch (std::get<0>(record).Type) {
    EXPORT(DnsARecord)
    EXPORT(DnsNSRecord)
    EXPORT(DnsCNAMERecord)
    EXPORT(DnsMXRecord)
    EXPORT(DnsAAAARecord)
    default: {
      RETURN_ON_ERROR(std::get<DnsUnknownRecord>(std::get<1>(record))
                          .Export(stream, labels));
    }
  }
#undef EXPORT

  uint16_t end_of_data = stream->CurrentByte() - length_location;
  CAUSE_ON_ERROR(stream->WriteAt<16>(end_of_data - 2, length_location));

  return base::OkStatus();
}

PacketStatus ExportRecords(WriteStream* stream,
                           const std::vector<PreambleAndRecord>& records,
                           LabelManager* labels) {
  for (const auto& record : records)
    RETURN_ON_ERROR(ExportRecord(stream, record, labels));
  return base::OkStatus();
}

}  // namespace _exporting

PacketStatus DnsPacket::Export(WriteStream* stream) {
  label_manager_->ResetWritePositions();
  RETURN_ON_ERROR(_exporting::ExportHeader(stream, *header_));
  RETURN_ON_ERROR(
      _exporting::ExportQuestions(stream, questions_, label_manager_.get()));
  RETURN_ON_ERROR(
      _exporting::ExportRecords(stream, answers_, label_manager_.get()));
  RETURN_ON_ERROR(
      _exporting::ExportRecords(stream, authorities_, label_manager_.get()));
  RETURN_ON_ERROR(
      _exporting::ExportRecords(stream, additional_, label_manager_.get()));
  return base::OkStatus();
}

namespace _importing {

PacketStatus::Or<std::vector<DnsQuestion>>
ImportQuestions(uint16_t qc, ReadStream* stream, LabelManager* labels) {
  std::vector<DnsQuestion> questions;
  for (uint16_t i = 0; i < qc; i++) {
    DnsQuestion question;
    ASSIGN_OR_ERROR(question.LabelSequence,
                    labels->ImportLabelSequence(stream));
    CAUSE_ON_ERROR(stream->Next<16>(&question.Type));
    CAUSE_ON_ERROR(stream->Next<16>(&question.Class));
    questions.push_back(std::move(question));
  }
  return questions;
}

PacketStatus::Or<DnsRecord> ImportRecord(ReadStream* stream,
                                         LabelManager* labels,
                                         uint16_t type,
                                         uint16_t length) {
  std::cout << "importing record: " << type << "\n";
  switch (type) {
    case DnsARecord::TYPE: {
      DnsARecord result;
      RETURN_ON_ERROR(result.Import(stream, labels));
      return DnsRecord{result};
    }
    case DnsNSRecord::TYPE: {
      DnsNSRecord result;
      RETURN_ON_ERROR(result.Import(stream, labels));
      return DnsRecord{result};
    }
    case DnsCNAMERecord::TYPE: {
      DnsCNAMERecord result;
      RETURN_ON_ERROR(result.Import(stream, labels));
      return DnsRecord{result};
    }
    case DnsMXRecord::TYPE: {
      DnsMXRecord result;
      RETURN_ON_ERROR(result.Import(stream, labels));
      return DnsRecord{result};
    }
    case DnsAAAARecord::TYPE: {
      DnsAAAARecord result;
      RETURN_ON_ERROR(result.Import(stream, labels));
      return DnsRecord{result};
    }
    default: {
      std::vector<uint8_t> data;
      while (length-- > 0) {
        uint8_t b;
        CAUSE_ON_ERROR(stream->Next<8>(&b));
        data.push_back(b);
      }
      return DnsRecord{DnsUnknownRecord{std::move(data)}};
    }
  }
}

PacketStatus::Or<std::vector<PreambleAndRecord>>
ImportRecords(uint16_t rc, ReadStream* stream, LabelManager* labels) {
  std::vector<PreambleAndRecord> records;
  for (uint16_t i = 0; i < rc; i++) {
    DnsRecordPreamble preamble;
    ASSIGN_OR_ERROR(preamble.LabelSequence,
                    labels->ImportLabelSequence(stream));
    CAUSE_ON_ERROR(stream->Next<16>(&preamble.Type));
    CAUSE_ON_ERROR(stream->Next<16>(&preamble.Class));
    CAUSE_ON_ERROR(stream->Next<32>(&preamble.TTL));
    CAUSE_ON_ERROR(stream->Next<16>(&preamble.Length));

    auto m_record =
        ImportRecord(stream, labels, preamble.Type, preamble.Length);
    if (m_record.has_error())
      return std::move(m_record).error().AddHere();
    records.push_back(std::make_tuple<DnsRecordPreamble, DnsRecord>(
        std::move(preamble), std::move(m_record).value()));
  }
  return records;
}

}  // namespace _importing

// static
PacketStatus::Or<DnsPacket> DnsPacket::Import(
    std::unique_ptr<ReadStream> stream) {
  DnsPacket result{0};
  RETURN_ON_ERROR(DnsPacketHeader::Import(result.header_.get(), stream.get()));
  ASSIGN_OR_ERROR(result.questions_,
                  _importing::ImportQuestions(result.header_->QC, stream.get(),
                                              result.label_manager_.get()));
  ASSIGN_OR_ERROR(result.answers_,
                  _importing::ImportRecords(result.header_->AC, stream.get(),
                                            result.label_manager_.get()));
  ASSIGN_OR_ERROR(result.authorities_,
                  _importing::ImportRecords(result.header_->NC, stream.get(),
                                            result.label_manager_.get()));
  ASSIGN_OR_ERROR(result.additional_,
                  _importing::ImportRecords(result.header_->DC, stream.get(),
                                            result.label_manager_.get()));
  return result;
}

namespace _rendering {

template <typename N>
std::string Hex(N numeric) {
  std::stringstream stream;
  stream << std::hex << "0x" << numeric;
  return stream.str();
}

template <size_t bits, typename N>
std::string Bits(N numeric) {
  std::stringstream stream;
  stream << std::hex << "0b" << std::bitset<bits>(numeric);
  return stream.str();
}

base::json::Array RenderQuestions(const std::vector<DnsQuestion>& qs) {
  std::vector<base::json::JSON> result;
  for (const DnsQuestion& q : qs) {
    std::map<std::string, base::json::JSON> fields;
    fields["Label"] = q.LabelSequence->Render();
    fields["Type"] = q.Type;
    fields["Class"] = q.Class;
    result.push_back(base::json::Object(std::move(fields)));
  }
  return base::json::Array(std::move(result));
}

base::json::Object RenderRecord(const DnsRecord& record) {
  std::map<std::string, base::json::JSON> blob;
  if (const auto *type = std::get_if<DnsARecord>(&record)) {
    return type->Render();
  } else if (const auto *type = std::get_if<DnsNSRecord>(&record)) {
    return type->Render();
  } else if (const auto *type = std::get_if<DnsCNAMERecord>(&record)) {
    return type->Render();
  } else if (const auto *type = std::get_if<DnsMXRecord>(&record)) {
    return type->Render();
  } else if (const auto *type = std::get_if<DnsAAAARecord>(&record)) {
    return type->Render();
  }
  return base::json::Object(std::move(blob));
}

base::json::Array RenderRecords(const std::vector<PreambleAndRecord>& rs) {
  std::vector<base::json::JSON> result;
  for (const auto& record : rs) {
    const DnsRecordPreamble& preamble = std::get<0>(record);
    std::map<std::string, base::json::JSON> fields;
    fields["Label"] = preamble.LabelSequence->Render();
    fields["Type"] = preamble.Type;
    fields["Class"] = preamble.Class;
    fields["TTL"] = preamble.TTL;
    fields["Length"] = preamble.Length;
    fields["Record"] = RenderRecord(std::get<1>(record));
    result.push_back(base::json::Object(std::move(fields)));
  }
  return base::json::Array(std::move(result));
}

}  // namespace _rendering

base::json::Object DnsPacket::Render() {
  std::map<std::string, base::json::JSON> header;
  std::stringstream stream;
  header["ID"] = _rendering::Hex(header_->ID);
  header["Type"] = (header_->QR ? "Query" : "Response");
  header["Opcode"] = _rendering::Bits<4>(header_->OP);
  header["Authoritative"] = (header_->AA ? "Yes" : "No");
  header["Truncated"] = (header_->TC ? "Yes" : "No");
  header["Recursion Desired"] = (header_->RD ? "Yes" : "No");
  header["Recursion Available"] = (header_->RA ? "Yes" : "No");
  header["Reserved"] = _rendering::Bits<3>(header_->RZ);
  header["Response Code"] = _rendering::Bits<4>(header_->RC);
  if (header_->QC) {
    header["QC"] = (int)header_->QC;
    header["Questions"] = _rendering::RenderQuestions(questions_);
  }
  if (header_->AC) {
    header["AC"] = (int)header_->AC;
    header["Answers"] = _rendering::RenderRecords(answers_);
  }
  if (header_->NC) {
    header["NC"] = (int)header_->NC;
    header["Authorities"] = _rendering::RenderRecords(authorities_);
  }
  if (header_->DC) {
    header["DC"] = (int)header_->DC;
    header["Additional"] = _rendering::RenderRecords(additional_);
  }
  return base::json::Object(std::move(header));
}

const DnsPacketHeader& DnsPacket::GetPacketHeader() const {
  return *header_;
}

DnsPacket DnsPacket::SetQuestionOrResponse(PacketType type) && {
  header_->QR = (type == PacketType::kQuestion ? 0 : 1);
  return std::move(*this);
}

DnsPacket DnsPacket::SetOpCode(uint8_t code) && {
  header_->OP = code;
  return std::move(*this);
}

DnsPacket DnsPacket::SetIsAuthoritative(bool authoritative) && {
  header_->AA = authoritative;
  return std::move(*this);
}

DnsPacket DnsPacket::SetIsTruncated(bool truncated) && {
  header_->TC = truncated;
  return std::move(*this);
}

DnsPacket DnsPacket::SetRecursionDesired(bool recursion) && {
  header_->RD = recursion;
  return std::move(*this);
}

DnsPacket DnsPacket::SetRecursionAvailable(bool recursion) && {
  header_->RA = recursion;
  return std::move(*this);
}

DnsPacket DnsPacket::SetResponseCode(uint8_t code) && {
  header_->RC = code;
  return std::move(*this);
}

DnsPacket DnsPacket::SetReserved(uint8_t reserved) && {
  header_->RZ = reserved;
  return std::move(*this);
}

std::optional<const DnsQuestion*> DnsPacket::GetQuestion(size_t q_num) {
  if (q_num > questions_.size())
    return std::nullopt;
  return &questions_[q_num];
}

std::optional<const PreambleAndRecord*> DnsPacket::GetAnswer(size_t a_num) {
  if (a_num > answers_.size())
    return std::nullopt;
  return &answers_[a_num];
}

std::optional<const PreambleAndRecord*> DnsPacket::GetAuthority(size_t a_num) {
  if (a_num > authorities_.size())
    return std::nullopt;
  return &authorities_[a_num];
}

std::optional<const PreambleAndRecord*> DnsPacket::GetAdditional(size_t a_num) {
  if (a_num > additional_.size())
    return std::nullopt;
  return &additional_[a_num];
}

PacketStatus::Or<DnsPacket> DnsPacket::AddQuestion(std::string name,
                                                   uint16_t Type,
                                                   uint16_t Class) {
  auto label = label_manager_->GetLabelSeq(name);
  if (!label.has_value())
    return std::move(label).error();

  DnsQuestion question = {std::move(label).value(), Type, Class};
  questions_.push_back(std::move(question));
  header_->QC++;
  return std::move(*this);
}

}  // namespace homedns