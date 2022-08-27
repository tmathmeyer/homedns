
#include <iostream>
#include <memory>

#include "homedns/bitstream.h"

#include <bitset>

void WriteTest() {
#define WRITEPRINT(bits, value)                                     \
  do {                                                              \
    auto ws = std::make_unique<homedns::WriteStream>(4);            \
    auto st = ws->Write<bits>(value);                               \
    if (!st.is_ok()) {                                              \
      st.Print();                                                   \
    } else {                                                        \
      auto rs = std::move(ws)->Convert();                           \
      uint32_t __tempvalue = 0;                                     \
      uint32_t __tempvalue2 = 0;                                    \
      auto st2 = rs->Read<32>(&__tempvalue, 0, 0);                  \
      auto st3 = rs->Read<bits>(&__tempvalue2, 0, 0);               \
      if (!st2.is_ok()) {                                           \
        st2.Print();                                                \
      } else {                                                      \
        std::cout << "Write<" << bits << ">(" << value              \
                  << ") = " << std::bitset<32>(__tempvalue) << " (" \
                  << std::bitset<bits>(__tempvalue2) << ")\n";      \
      }                                                             \
    }                                                               \
  } while (0)

  WRITEPRINT(6, 0xAA);
}

void ReadTest() {
  // uint8_t value[4] = {0xDB, 0x6D, 0xB6, 0xDB};
  uint8_t value[4] = {0xAA, 0x00, 0x00, 0x00};

  std::unique_ptr<homedns::ReadStream> bs =
      std::make_unique<homedns::ReadStream>(4, value);

#define PRINTREAD(bits, by, bi)                                            \
  do {                                                                     \
    uint64_t __tempvalue = 0;                                              \
    auto st = bs->Read<bits>(&__tempvalue, by, bi);                        \
    if (!st.is_ok()) {                                                     \
      st.Print();                                                          \
    } else {                                                               \
      std::cout << "Read<" << bits << ">(" << by << ", " << bi << ") = 0b" \
                << std::bitset<bits>(__tempvalue) << "\n";                 \
    }                                                                      \
  } while (0)

  PRINTREAD(32, 0, 0);
  PRINTREAD(8, 0, 0);
  PRINTREAD(3, 0, 0);
  PRINTREAD(3, 0, 1);
  PRINTREAD(3, 0, 2);
  PRINTREAD(3, 0, 3);
  PRINTREAD(7, 0, 0);
  PRINTREAD(7, 0, 1);
  PRINTREAD(7, 0, 2);
  PRINTREAD(7, 0, 3);

#define PRINTNEXT(bits)                                    \
  do {                                                     \
    uint64_t __tempvalue = 0;                              \
    auto st = bs->Next<bits>(&__tempvalue);                \
    if (!st.is_ok()) {                                     \
      st.Print();                                          \
    } else {                                               \
      std::cout << "Next<" << bits << ">() = 0b"           \
                << std::bitset<bits>(__tempvalue) << "\n"; \
    }                                                      \
  } while (0)

  PRINTNEXT(4);
  PRINTNEXT(4);
  PRINTNEXT(4);
  PRINTNEXT(4);
  PRINTNEXT(4);
}

int main() {
  ReadTest();
  WriteTest();
}