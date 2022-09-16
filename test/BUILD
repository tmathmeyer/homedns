langs("C")

cc_binary (
  name = "bitstream",
  srcs = [
    "bitstream.cc"
  ],
  include = [
    "//homedns:include",
  ],
  deps = [
    "//base/status:status",
  ],
)

cc_binary (
  name = "packet",
  srcs = [
    "packet.cc"
  ],
  include = [
    "//homedns:include",
  ],
  deps = [
    "//homedns:libdns",
  ],
)

cc_binary (
  name = "query",
  srcs = [
    "query.cc"
  ],
  include = [
    "//homedns:include",
  ],
  deps = [
    "//homedns:libdns",
  ],
)