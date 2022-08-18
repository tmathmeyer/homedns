langs("C")

cpp_header (
  name = "include",
  srcs = [
    "bitstream.h",
  ],
  deps = [
    "//base/status:include",
  ],
)

cpp_binary (
  name = "bitstream_test",
  srcs = [
    "bitstream_test.cc"
  ],
  deps = [
    ":include",
    "//base/status:status",
  ],
)