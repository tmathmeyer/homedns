langs("C")

cpp_binary (
  name = "bitstream",
  srcs = [
    "bitstream.cc"
  ],
  deps = [
    "//homedns:include",
    "//base/status:status",
  ],
)

cpp_binary (
  name = "packet",
  srcs = [
    "packet.cc"
  ],
  deps = [
    "//homedns:include",
    "//homedns:packet",
    "//homedns:records",
    "//base/status:status",
  ],
)

cpp_binary (
  name = "query",
  srcs = [
    "query.cc"
  ],
  deps = [
    "//homedns:include",
    "//homedns:packet",
    "//base/status:status",
    "//base/json:json_io",
  ],
)