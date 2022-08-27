langs("C")

cpp_header (
  name = "include",
  srcs = [
    "bitstream.h",
    "labels.h",
    "packet.h",
    "records.h",
    "status.h",
  ],
  deps = [
    "//base/status:include",
  ],
)

cpp_object (
  name = "packet",
  srcs = [
    "packet.cc",
  ],
  deps = [
    ":include",
    ":labels",
    ":records",
  ],
)

cpp_object (
  name = "records",
  srcs = [
    "records.cc",
  ],
  deps = [
    ":include",
  ],
)

cpp_object (
  name = "labels",
  srcs = [
    "labels.cc",
  ],
  deps = [
    ":include",
  ],
)
