
#include <iostream>
#include <memory>

#include "bitstream.h"

#include <bitset>

void WriteTest() {
  std::unique_ptr<homedns::WriteStream> bs =
      std::make_unique<homedns::WriteStream>(4);
}


void ReadTest() {
  uint8_t value[4] = {0xDB, 0x6D, 0xB6, 0xDB};

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