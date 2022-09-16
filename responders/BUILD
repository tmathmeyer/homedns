langs("C")

cpp_header (
  name = "include",
  srcs = [
    "responders.h",
  ],
  deps = [
    "//homedns:include",
  ],
)

cc_object (
  name = "responders",
  srcs = [
    "a_record.cc",
    "aaaa_record.cc",
    "cname_record.cc",
    "loc_record.cc",
    "mx_record.cc",
    "ns_record.cc",
    "rp_record.cc",
    "soa_record.cc",
    "txt_record.cc",
  ],
  includes = [
    ":include",
  ],
)