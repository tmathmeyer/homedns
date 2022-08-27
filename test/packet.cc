
#include "homedns/packet.h"
#include "base/json/json_io.h"
#include "homedns/bitstream.h"

void DumpBuffer(homedns::ReadStream* rs) {
  for (size_t i = 0; i < rs->Size(); i++) {
    uint8_t value;
    auto st = rs->Next<8>(&value);
    if (!st.is_ok()) {
      std::cout << "\n";
      st.Print();
      exit(1);
    }
    printf("%02X ", value);
    if (i % 8 == 7) {
      std::cout << " ";
    }
    if (i % 32 == 31) {
      std::cout << "\n";
    }
  }
  std::cout << "\n";
}

void BuildPacket() {
  homedns::DnsPacket packet =
      homedns::DnsPacket::Create(0x862a)
          .SetQuestionOrResponse(homedns::DnsPacket::PacketType::kQuestion)
          .SetOpCode(0)
          .SetIsAuthoritative(0)
          .SetIsTruncated(0)
          .SetRecursionDesired(1)
          .SetRecursionAvailable(0)
          .SetResponseCode(0)
          .SetReserved(2)
          .AddQuestion("google.com", homedns::DnsARecord::TYPE, 0x01)
          .Unwrap()
          .AddRecord<homedns::DnsPacket::RecordType::kAnswer>(
              "google.com", 0x01, 0xDEADBEEF, homedns::DnsARecord{{192, 168, 1, 1}})
          .Unwrap()
          .AddRecord<homedns::DnsPacket::RecordType::kAnswer>(
              "google.com", 0x01, 0x12345678, homedns::DnsNSRecord{"foo.bar"})
          .Unwrap();

  std::cout << packet.Render() << "\n";

  auto ws = std::make_unique<homedns::WriteStream>(512);
  auto ext = packet.Export(ws.get());
  if (!ext.is_ok()) {
    ext.Print();
    exit(1);
  }

  auto rs = ws->Convert();
  DumpBuffer(rs.get());
}


void ImportPacket() {
  uint8_t query[44] = {
      // 12 byte header
      // 1 question
      // 1 answer
      0x86, 0x2a, 0x81, 0x80, 0x00, 0x01, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00,
      // Questions [1]:

      // label seq:
      // [6] g     O     O     g     l     e
      0x06, 0x67, 0x6f, 0x6f, 0x67, 0x6c, 0x65,
      // [3] c     o     m
      0x03, 0x63, 0x6f, 0x6d,
      // [0]
      0x00,

      // Type=1   class=1
      0x00, 0x01, 0x00, 0x01,

      // label seq:
      // offset to 0xc = 12
      0xc0, 0x0c,

      // Type=1   class=1     ttl=293                 // len=4
      0x00, 0x01, 0x00, 0x01, 0x00, 0x00, 0x01, 0x25, 0x00, 0x04,

      // len = 4, 4 bytes of data
      0xd8, 0x3a, 0xd3, 0x8e};

  auto m_packet = homedns::DnsPacket::Import(
      std::make_unique<homedns::ReadStream>(44, query));
  if (!m_packet.has_value()) {
    std::move(m_packet).error().Print();
    exit(1);
  }
  homedns::DnsPacket packet = std::move(m_packet).value();
  std::cout << packet.Render() << "\n";

  /*
  if (packet.GetAnswerType(0) != homedns::DnsARecord::TYPE) {
    printf("Not an A record! (%i)\n", packet.GetAnswerType(0));
    exit(1);
  }
  auto record = packet.GetAnswer<homedns::DnsARecord>(0);
  if (record.has_value()) {
    homedns::DnsARecord rec = std::move(record).value();
    std::cout << "A ip: " << std::dec << static_cast<uint16_t>(rec.IP[0]) << "."
              << static_cast<uint16_t>(rec.IP[1]) << "."
              << static_cast<uint16_t>(rec.IP[2]) << "."
              << static_cast<uint16_t>(rec.IP[3]) << "\n";
  } else {
    std::move(record).error().Print();
  }
  */
}


int main() {
  // RequestHeader();
  // puts("\n\n");
  // ResponseHeader();
  // BuildPacket();
  ImportPacket();
}