#pragma once

#include <bitset>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <sstream>
#include <type_traits>

#include "status.h"

namespace homedns {

#define AssignOrRetError(d_ptr, expr)                 \
  do {                                                \
    auto v_or_error = (expr);                         \
    if (v_or_error.has_value())                       \
      *d_ptr = std::move(v_or_error).value();         \
    else                                              \
      return std::move(v_or_error).error().AddHere(); \
  } while (0)

class ReadStream {
 private:
  uint8_t bitbuffer_ = 0;
  size_t bitlag_ = 0;
  size_t next_ = 0;
  size_t size_ = 0;
  uint8_t* buffer_ = nullptr;
  bool owns_buffer_ = false;

  BitstreamStatus::Or<uint8_t> ReadByte(size_t index) const {
    if (index >= size_) {
      return BitstreamStatus(BitstreamStatus::Codes::kOutOfBounds)
          .WithData("byte", static_cast<int>(index))
          .WithData("length", static_cast<int>(size_));
    }
    return BitstreamStatus::Or<uint8_t>(buffer_[index]);
  }

 public:
  ~ReadStream() {
    if (owns_buffer_)
      free(buffer_);
  }

  ReadStream(size_t size, void* memory, bool owns_buffer = false) {
    size_ = size;
    buffer_ = static_cast<uint8_t*>(memory);
    owns_buffer_ = owns_buffer;
  }

  size_t CurrentByte() const { return next_; }

  size_t Size() const { return size_; }

  const uint8_t* GetBuffer() const { return buffer_; }

  template <size_t bits, typename T>
  BitstreamStatus Read(T* into, size_t byte = 0, size_t bit = 0) const {
    static_assert(sizeof(T) * 8 >= bits);
    static_assert(std::is_integral_v<T>);
    if (bit > 7) {
      return BitstreamStatus::Codes::kInvalidBitOffset;
    }
    T buffer = 0;
    size_t bitsread = bits;
    size_t bitoffset = 8 - bit;
    size_t i = 0;
    uint8_t byte_value;

    AssignOrRetError(&byte_value, ReadByte(byte + i));
    while (bitsread > 0) {
      bitsread--;
      buffer <<= 1;
      buffer |= ((byte_value >> (bitoffset - 1)) & 0x01);
      if (bitoffset == 0) {
        bitoffset = 8;
        i++;
        if (bitsread)
          AssignOrRetError(&byte_value, ReadByte(byte + i));
      } else {
        bitoffset--;
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
    while (bitcount) {
      // std::cout << "Reading " << bitcount << " bits\n";
      if (bitlag_ >= bitcount) {
        // puts("  bitbuffer exceedes remainder");
        // std::cout << "  bitbuffer[" << bitlag_ << ", "
        //           << std::bitset<8>(bitbuffer_) << "]\n";
        T tbuffer = bitbuffer_;
        tbuffer >>= (8 - bitcount);
        bitlag_ -= bitcount;
        bitbuffer_ <<= bitcount;
        bitcount = 0;
        buffer_msb = 0;
        buffer |= (tbuffer << buffer_msb);
        // std::cout << "  bitbuffer[" << bitlag_ << ", "
        //           << std::bitset<8>(bitbuffer_) << "]\n";
      } else if (bitlag_) {
        // std::cout << "  bitbuffer[" << bitlag_ << ", "
        //           << std::bitset<8>(bitbuffer_) << "] can't satisfy read\n";
        T tbuffer = bitbuffer_;
        tbuffer >>= (8 - bitlag_);
        bitcount -= bitlag_;
        buffer_msb -= bitlag_;
        bitlag_ = 0;
        bitbuffer_ = 0;
        buffer |= (tbuffer << buffer_msb);
      }

      while (bitcount >= 8) {
        // puts("  reading full byte");
        T temp;
        AssignOrRetError(&temp, ReadByte(next_));
        next_++;
        bitcount -= 8;
        buffer_msb -= 8;
        buffer |= (temp << buffer_msb);
        // std::cout << "  >>" << std::bitset<sizeof(T) * 8>(buffer) << "\n";
      }

      if (bitcount) {
        // std::cout << "  Remaining " << bitcount << " bits, filling buffer\n
        // ";
        AssignOrRetError(&bitbuffer_, ReadByte(next_));
        next_++;
        bitlag_ = 8;
      }
    }

    *into = buffer;
    return base::OkStatus();
  }
};

class WriteStream {
 private:
  uint8_t* buffer_ = nullptr;
  bool allocated_ = false;
  size_t size_ = 0;
  size_t byte_ = 0;
  size_t bit_ = 7;

  BitstreamStatus WriteNextBit(uint8_t bit) {
    if (byte_ >= size_) {
      std::stringstream msg;
      msg << "OOB Write on byte: " << byte_ << ", length: " << size_;
      return {BitstreamStatus::Codes::kOutOfBounds, msg.str()};
    }
    buffer_[byte_] |= (bit << bit_);
    if (bit_) {
      bit_--;
    } else {
      bit_ = 7;
      byte_++;
    }
    return base::OkStatus();
  }

 public:
  ~WriteStream() {
    if (allocated_)
      free(buffer_);
  }

  WriteStream(size_t size, void* buffer = nullptr) {
    size_ = size;
    if (buffer == nullptr) {
      buffer_ = static_cast<uint8_t*>(malloc(size));
      if (buffer_ == nullptr) {
        std::cout << "Allocation size " << size << " failed! aborting!\n";
        exit(1);
      }
      memset(buffer_, 0, size);
      allocated_ = true;
    } else {
      buffer_ = static_cast<uint8_t*>(buffer);
    }
  }

  size_t CurrentByte() const { return byte_; }

  std::unique_ptr<ReadStream> Convert() {
    uint8_t* buffer = static_cast<uint8_t*>(malloc(byte_));
    memcpy(buffer, buffer_, byte_);
    return std::make_unique<ReadStream>(byte_, buffer, true);
  }

  template <size_t bits, typename T>
  BitstreamStatus Write(const T& from) {
    static_assert(sizeof(T) * 8 >= bits);
    static_assert(std::is_integral_v<T>);
    for (int i = bits; i; i--) {
      uint8_t bit = (from >> (i - 1)) & 0x01;
      auto st = WriteNextBit(bit);
      if (!st.is_ok())
        return std::move(st).AddHere();
    }
    return base::OkStatus();
  }

  template<size_t bits, typename T>
  BitstreamStatus WriteAt(const T& from, size_t byte=0) {
    size_t o_byte = byte_;
    size_t o_bit_ = bit_;
    byte_ = byte;
    bit_ = 7;
    BitstreamStatus st = Write<bits, T>(from);
    byte_ = o_byte;
    bit_ = o_bit_;
    return std::move(st).AddHere();
  }
};

}  // namespace homedns