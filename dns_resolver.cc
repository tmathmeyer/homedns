


class DnsServer : public UDPServer {
 public:
  DnsServer() : UDPServer(53) {}
  ~DnsServer() override = default;

  std::vector<uint8_t> OnData(std::vector<uint8_t> data) override {

  }
}

int main() {
  UDPServer server(53);
  server.Start();
}