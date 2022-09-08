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

cpp_header (
  name = "udp_include",
  srcs = [
    "udp_server.h",
  ],
  deps = [
    ":include",
    "//base/bind:include",
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

cpp_object (
  name = "udp_server",
  srcs = [
    "udp_server.cc",
  ],
  deps = [
    ":include",
    ":udp_include",
  ],
)


cpp_binary (
  name = "dns_resolver",
  srcs = [
    "dns_resolver.cc"
  ],
  deps = [
    ":udp_include",
    ":udp_server",
    ":packet",
    ":records",
    ":labels",
    "//base/status:status",
  ],
)