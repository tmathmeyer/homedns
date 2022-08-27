#pragma once

#include "base/status/status.h"

namespace homedns {

struct PacketStatusSpec {
  enum class Codes : base::StatusCodeType {
    kOk,
    kParsingError,
    kIndexOutOfRange,
  };

  static base::StatusGroupType Group() { return "PacketStatus"; }
};

using PacketStatus = base::TypedStatus<PacketStatusSpec>;

struct BitstreamErrorSpec {
  enum class Codes : base::StatusCodeType {
    kOk,
    kOutOfBounds,
    kInvalidBitOffset,
  };

  static base::StatusGroupType Group() { return "BitstreamStatus"; }
};

using BitstreamStatus = base::TypedStatus<BitstreamErrorSpec>;

}  // namespace homedns