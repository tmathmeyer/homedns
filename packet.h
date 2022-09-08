#pragma once

#include "bitstream.h"
#include "labels.h"
#include "records.h"
#include "status.h"

namespace homedns {

struct DnsQuestion {
  std::unique_ptr<DnsLabelSeq> LabelSequence;
  uint16_t Type;
  uint16_t Class;
};

struct DnsRecordPreamble {
  std::unique_ptr<DnsLabelSeq> LabelSequence;
  uint16_t Type;
  uint16_t Class;
  uint32_t TTL;
  uint16_t Length;
};

struct DnsPacketHeader {
  uint16_t ID : 16;  // ID
  uint16_t QR : 1;   // Question / Response
  uint16_t OP : 4;   // Operation Code
  uint16_t AA : 1;   // ?AuthoritativeAnswer
  uint16_t TC : 1;   // ?TruncatedMessage
  uint16_t RD : 1;   // ?RecursionDesired
  uint16_t RA : 1;   // ?RecursionAvailable
  uint16_t RZ : 3;   // Reserved
  uint16_t RC : 4;   // Response Code
  uint16_t QC : 16;  // Question Count [DnsQuestion]
  uint16_t AC : 16;  // Answer Count [DnsRecord]
  uint16_t NC : 16;  // Authority Count [DnsRecord]
  uint16_t DC : 16;  // Additional Count [DnsRecord]

  static PacketStatus Import(DnsPacketHeader* header, ReadStream* bitstream);
} __attribute__((packed));

static_assert(sizeof(DnsPacketHeader) == 12);

using PreambleAndRecord = std::tuple<DnsRecordPreamble, DnsRecord>;

class DnsPacket {
 public:
  /* inner types */
  enum class PacketType { kQuestion, kResponse };
  enum class RecordType { kAnswer, kAuthority, kAdditional };

 private:
  /* label manager owned directly */
  std::unique_ptr<LabelManager> label_manager_;

  /* data fields that get serialized / deserialized */
  std::unique_ptr<DnsPacketHeader> header_;
  std::vector<DnsQuestion> questions_;
  std::vector<PreambleAndRecord> answers_;
  std::vector<PreambleAndRecord> authorities_;
  std::vector<PreambleAndRecord> additional_;

 public:
  /* lifetime */
  ~DnsPacket() = default;
  DnsPacket(DnsPacket&&) = default;
  void operator=(DnsPacket&&);
  explicit DnsPacket(uint16_t ID);
  static DnsPacket Create(uint16_t ID);

  /* Importers and exporters */
  PacketStatus Export(WriteStream* stream);
  static PacketStatus::Or<DnsPacket> Import(std::unique_ptr<ReadStream> stream);
  base::json::Object Render();

  /* Getters and setters */
  const DnsPacketHeader& GetPacketHeader() const;
  DnsPacket SetQuestionOrResponse(PacketType type) &&;
  DnsPacket SetOpCode(uint8_t code) &&;
  DnsPacket SetIsAuthoritative(bool authoritative) &&;
  DnsPacket SetIsTruncated(bool truncated) &&;
  DnsPacket SetRecursionDesired(bool recursion) &&;
  DnsPacket SetRecursionAvailable(bool recursion) &&;
  DnsPacket SetResponseCode(uint8_t code) &&;
  DnsPacket SetReserved(uint8_t reserved) &&;

  size_t GetNumQuestions() const { return questions_.size(); }
  size_t GetNumAnswers() const { return answers_.size(); }
  size_t GetNumAuthorities() const { return authorities_.size(); }
  size_t GetNumAdditional() const { return additional_.size(); }

  std::optional<const DnsQuestion*> GetQuestion(size_t q_num);
  std::optional<const PreambleAndRecord*> GetAnswer(size_t a_num);
  std::optional<const PreambleAndRecord*> GetAuthority(size_t a_num);
  std::optional<const PreambleAndRecord*> GetAdditional(size_t a_num);

  PacketStatus::Or<DnsPacket> AddQuestion(std::string name,
                                          uint16_t Type,
                                          uint16_t Class);
  template <RecordType R, typename T>
  PacketStatus::Or<DnsPacket> AddRecord(std::string Name,
                                        uint16_t Class,
                                        uint32_t TTL,
                                        T Record) {
    std::vector<PreambleAndRecord>* vec;
    if constexpr (R == RecordType::kAnswer)
      vec = &answers_;
    else if constexpr (R == RecordType::kAuthority)
      vec = &authorities_;
    else if constexpr (R == RecordType::kAdditional)
      vec = &additional_;
    else
      return PacketStatus::Codes::kIndexOutOfRange;

    auto label = label_manager_->GetLabelSeq(Name);
    if (!label.has_value())
      return std::move(label).error();

    vec->push_back(std::make_tuple<DnsRecordPreamble, DnsRecord>(
        {std::move(label).value(), T::TYPE, Class, TTL, 0}, std::move(Record)));

    if constexpr (R == RecordType::kAnswer)
      header_->AC++;
    else if constexpr (R == RecordType::kAuthority)
      header_->NC++;
    else if constexpr (R == RecordType::kAdditional)
      header_->DC++;

    return std::move(*this);
  }
};

}  // namespace homedns