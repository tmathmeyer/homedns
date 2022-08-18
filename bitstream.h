#pragma once

#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <type_traits>

#include "base/status/status.h"

#include <bitset>
#include <iostream>

namespace homedns {

enum class BitstreamType {
  kRead,
  kWrite,
};

struct BitstreamErrorSpec {
  enum class Codes : base::StatusCodeType {
    kOk,
    kOutOfBounds,
    kInvalidBitOffset,
  };

  static base::StatusGroupType Group() { return "BitstreamStatus"; }
};

using BitstreamStatus = base::TypedStatus<BitstreamErrorSpec>;

#define AssignOrRetError(d_ptr, expr)                 \
  do {                                                \
    auto v_or_error = (expr);                         \
    if (v_or_error.has_value())                       \
      *d_ptr = std::move(v_or_error).value();         \
    else                                              \
      return std::move(v_or_error).error().AddHere(); \
  } while (0)

template <BitstreamType B>
class Bitstream {
 private:
  BitstreamStatus::Or<uint8_t> ReadByte(size_t index) {
    if (index >= size_) {
      return BitstreamStatus(BitstreamStatus::Codes::kOutOfBounds)
          .WithData("byte", static_cast<int>(index))
          .WithData("length", static_cast<int>(size_));
    }
    return BitstreamStatus::Or<uint8_t>(buffer_[index]);
  }

 public:
  template <BitstreamType T = B>
  Bitstream(std::enable_if<T == BitstreamType::kWrite, size_t>::type size) {
    size_ = size;
    buffer_ = static_cast<uint8_t*>(malloc(size));
    memset(buffer_, 0, size);
    allocated_ = true;
  }

  template <BitstreamType T = B>
  Bitstream(std::enable_if<T == BitstreamType::kRead, size_t>::type size,
            void* memory) {
    size_ = size;
    buffer_ = static_cast<uint8_t*>(memory);
  }

  ~Bitstream() {
    if (allocated_)
      free(buffer_);
  }

  template <size_t bits, typename T>
  BitstreamStatus Read(T* into, size_t byte = 0, size_t bit = 0) {
    static_assert(sizeof(T) * 8 >= bits);
    static_assert(std::is_integral_v<T>);
    if (bit > 7) {
      return BitstreamStatus::Codes::kInvalidBitOffset;
    }
    T buffer = 0;
    size_t bitsread = bits;
    size_t bitoffset = bit;
    size_t i = 0;
    uint8_t byte_value;
    AssignOrRetError(&byte_value, ReadByte(byte + i));
    while (bitsread-- > 0) {
      buffer <<= 1;
      buffer |= ((byte_value >> bitoffset) & 0x01);
      if (++bitoffset == 8) {
        bitoffset = 0;
        i++;
        if (bitsread)
          AssignOrRetError(&byte_value, ReadByte(byte + i));
      }
    }
    *into = buffer;
    return base::OkStatus();
  }

  template <size_t bits, typename T>
  BitstreamStatus Next(T* into) {
    static_assert(sizeof(T) * 8 >= bits);
    static_assert(std::is_integral_v<T>);

    T buffer = 0;
    size_t bitcount = bits;
    size_t buffer_msb = bits;

  read_cycle:
    //std::cout << "Reading " << bitcount << " bits\n";
    if (bitlag_ >= bitcount) {
      //puts("  bitbuffer exceedes remainder");
      //std::cout << "  bitbuffer[" << bitlag_ << ", "
      //          << std::bitset<8>(bitbuffer_) << "]\n";
      T tbuffer = bitbuffer_;
      tbuffer >>= (8 - bitcount);
      bitlag_ -= bitcount;
      bitbuffer_ <<= bitcount;
      bitcount = 0;
      buffer_msb = 0;
      buffer |= (tbuffer << buffer_msb);
      //std::cout << "  bitbuffer[" << bitlag_ << ", "
      //          << std::bitset<8>(bitbuffer_) << "]\n";
    } else if (bitlag_) {
      //std::cout << "  bitbuffer[" << bitlag_ << ", "
      //          << std::bitset<8>(bitbuffer_) << "] can't satisfy read\n";
      T tbuffer = bitbuffer_;
      tbuffer >>= (8 - bitlag_);
      bitcount -= bitlag_;
      buffer_msb -= bitlag_;
      bitlag_ = 0;
      bitbuffer_ = 0;
      buffer |= (tbuffer << buffer_msb);
    }

    while (bitcount >= 8) {
      //puts("  reading full byte");
      T temp;
      AssignOrRetError(&temp, ReadByte(next_));
      next_++;
      bitcount -= 8;
      buffer_msb -= 8;
      buffer |= (temp << buffer_msb);
      //std::cout << "  >>" << std::bitset<sizeof(T) * 8>(buffer) << "\n";
    }

    if (bitcount) {
      //std::cout << "  Remaining " << bitcount << " bits, filling buffer\n  ";
      AssignOrRetError(&bitbuffer_, ReadByte(next_));
      next_++;
      bitlag_ = 8;
      goto read_cycle;
    }

    *into = buffer;
    return base::OkStatus();
  }

  template <size_t bits, typename T>
  BitstreamStatus Write(const T& from) {
    static_assert(sizeof(T) * 8 >= bits);
    static_assert(std::is_integral_v<T>);
    return BitstreamStatus::Codes::kOutOfBounds;
  }

 private:
  uint8_t bitbuffer_ = 0;
  size_t bitlag_ = 0;
  size_t next_ = 0;
  size_t size_ = 0;
  uint8_t* buffer_ = nullptr;

  bool allocated_ = false;
};

using WriteStream = Bitstream<BitstreamType::kWrite>;
using ReadStream = Bitstream<BitstreamType::kRead>;

}  // namespace homedns