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
  includes = [
    ":include",
    "//base/bind:include",
  ],
)

cc_object (
  name = "libdns",
  srcs = [
    "labels.cc",
    "packet.cc",
    "records.cc",
  ],
  includes = [
    ":include",
  ],
  deps = [
    "//base/status:status",
  ],
)

cc_object (
  name = "libudp",
  srcs = [
    "udp_server.cc",
  ],
  includes = [
    ":udp_include",
  ],
)

cc_binary (
  name = "dns_resolver",
  srcs = [
    "dns_resolver.cc",
  ],
  includes = [
    ":udp_include",
    ":include",
    "//homedns/responders:include",
  ],
  deps = [
    ":libdns",
    ":libudp",
    "//homedns/responders:responders",
  ],
)