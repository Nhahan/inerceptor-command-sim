#include <cassert>
#include <cstdlib>
#include <cctype>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>

#include "tests/support/temp_repo.hpp"

#if !defined(_WIN32)
#include <sys/wait.h>
#endif

namespace {

std::string read_text(const std::filesystem::path& file) {
    std::ifstream in(file);
    std::stringstream buffer;
    buffer << in.rdbuf();
    return buffer.str();
}

void write_text(const std::filesystem::path& file, const std::string& text) {
    std::ofstream out(file);
    out << text;
}

int parse_named_int(std::string_view text, std::string_view key) {
    const auto token = std::string(key) + "=";
    const auto start = text.find(token);
    if (start == std::string_view::npos) {
        return -1;
    }
    auto pos = start + token.size();
    std::string value;
    while (pos < text.size() && std::isdigit(static_cast<unsigned char>(text[pos]))) {
        value.push_back(text[pos++]);
    }
    return value.empty() ? -1 : std::stoi(value);
}

}  // namespace

int main() {
    namespace fs = std::filesystem;

    const auto temp_root = icss::testsupport::make_temp_configured_repo("icss_cli_precedence_");
    write_text(
        temp_root / "configs/server.example.yaml",
        "bind_host: 127.0.0.2\n"
        "tcp_frame_format: binary\n"
        "tick_rate_hz: 11\n"
        "udp_max_batch_snapshots: 3\n"
        "udp_send_latest_only: false\n"
        "ports:\n"
        "  tcp: 4100\n"
        "  udp: 4101\n"
        "heartbeat:\n"
        "  interval_ms: 700\n"
        "  timeout_ms: 1400\n"
        "session:\n"
        "  max_clients: 9\n");
    write_text(
        temp_root / "configs/scenario.example.yaml",
        "name: basic_intercept_training\n"
        "description: precedence smoke\n"
        "targets: 1\n"
        "assets: 1\n"
        "enable_replay: true\n"
        "telemetry_interval_ms: 250\n");

    const fs::path output_file = temp_root / "server-output.txt";
    const std::string server_path = (fs::path{ICSS_REPO_ROOT} / "build/icss_server").string();
    const std::string command =
        "\"" + server_path + "\""
        " --backend socket_live"
        " --repo-root \"" + temp_root.string() + "\""
        " --bind-host 127.0.0.1"
        " --tcp-port 0"
        " --udp-port 0"
        " --tcp-frame-format json"
        " --tick-limit 2"
        " --tick-rate-hz 30"
        " --telemetry-interval-ms 150"
        " --heartbeat-interval-ms 600"
        " --heartbeat-timeout-ms 1800"
        " --udp-max-batch-snapshots 1"
        " --udp-send-latest-only true"
        " --max-clients 4"
        " > \"" + output_file.string() + "\" 2>&1";

    const int system_result = std::system(command.c_str());
#if !defined(_WIN32)
    assert(WIFEXITED(system_result));
    assert(WEXITSTATUS(system_result) == 0);
#else
    assert(system_result == 0);
#endif

    const auto output = read_text(output_file);
    assert(output.find("tick_rate_hz=30") != std::string::npos);
    assert(output.find("bind_host=127.0.0.1") != std::string::npos);
    assert(output.find("frame_format=json") != std::string::npos);
    assert(output.find("telemetry_interval_ms=150") != std::string::npos);
    assert(output.find("heartbeat_interval_ms=600") != std::string::npos);
    assert(output.find("heartbeat_timeout_ms=1800") != std::string::npos);
    assert(output.find("udp_send_latest_only=true") != std::string::npos);
    assert(output.find("udp_max_batch_snapshots=1") != std::string::npos);
    assert(output.find("max_clients=4") != std::string::npos);
    const auto tcp_port = parse_named_int(output, "tcp_port");
    const auto udp_port = parse_named_int(output, "udp_port");
    assert(tcp_port > 0);
    assert(udp_port > 0);
    assert(tcp_port != 4100);
    assert(udp_port != 4101);

    assert(output.find("bind_host=127.0.0.2") == std::string::npos);
    assert(output.find("frame_format=binary") == std::string::npos);
    assert(output.find("tick_rate_hz=11") == std::string::npos);
    assert(output.find("telemetry_interval_ms=250") == std::string::npos);
    assert(output.find("heartbeat_interval_ms=700") == std::string::npos);
    assert(output.find("heartbeat_timeout_ms=1400") == std::string::npos);
    assert(output.find("udp_max_batch_snapshots=3") == std::string::npos);
    assert(output.find("max_clients=9") == std::string::npos);

    fs::remove_all(temp_root);
    return 0;
}
