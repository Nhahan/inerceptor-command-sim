#include "icss/core/runtime.hpp"

#include <filesystem>
#include <fstream>
#include <string>
#include <string_view>

#include "icss/net/transport.hpp"

namespace icss::core {
namespace {

constexpr std::string_view kRuntimeLogSchemaVersion = "icss-runtime-log-v1";

std::string escape_json(std::string_view input) {
    std::string escaped;
    escaped.reserve(input.size());
    for (char ch : input) {
        const auto uch = static_cast<unsigned char>(ch);
        switch (ch) {
        case '\\': escaped += "\\\\"; break;
        case '"': escaped += "\\\""; break;
        case '\n': escaped += "\\n"; break;
        case '\r': escaped += "\\r"; break;
        case '\t': escaped += "\\t"; break;
        default:
            if (uch < 0x20U) {
                constexpr char kHex[] = "0123456789ABCDEF";
                escaped += "\\u00";
                escaped += kHex[(uch >> 4U) & 0x0FU];
                escaped += kHex[uch & 0x0FU];
            } else {
                escaped += ch;
            }
            break;
        }
    }
    return escaped;
}

void drive_baseline_transport_sequence(icss::net::TransportBackend& transport, const icss::core::RuntimeConfig& config) {
    transport.connect_client(icss::core::ClientRole::CommandConsole, 101U);
    transport.connect_client(icss::core::ClientRole::TacticalViewer, 201U);
    if (!transport.start_scenario().accepted) {
        throw std::runtime_error("baseline start_scenario rejected");
    }
    if (!transport.dispatch(icss::protocol::TrackRequestPayload{{1001U, 101U, 1U}, "target-alpha"}).accepted) {
        throw std::runtime_error("baseline track_request rejected");
    }
    transport.advance_tick();
    if (!transport.dispatch(icss::protocol::AssetActivatePayload{{1001U, 101U, 2U}, "asset-interceptor"}).accepted) {
        throw std::runtime_error("baseline asset_activate rejected");
    }
    transport.disconnect_client(icss::core::ClientRole::TacticalViewer, "viewer reconnect exercised for resilience evidence");
    transport.connect_client(icss::core::ClientRole::TacticalViewer, 201U);
    if (!transport.dispatch(icss::protocol::CommandIssuePayload{{1001U, 101U, 3U}, "asset-interceptor", "target-alpha"}).accepted) {
        throw std::runtime_error("baseline command_issue rejected");
    }
    for (int i = 0; i < config.scenario.engagement_timeout_ticks && !transport.latest_snapshot().judgment.ready; ++i) {
        transport.advance_tick();
    }
}

void drive_baseline_session_sequence(icss::core::SimulationSession& session, const icss::core::RuntimeConfig& config) {
    session.connect_client(icss::core::ClientRole::CommandConsole, 101U);
    session.connect_client(icss::core::ClientRole::TacticalViewer, 201U);
    if (!session.start_scenario().accepted) {
        throw std::runtime_error("baseline start_scenario rejected");
    }
    if (!session.request_track().accepted) {
        throw std::runtime_error("baseline track_request rejected");
    }
    session.advance_tick();
    if (!session.activate_asset().accepted) {
        throw std::runtime_error("baseline asset_activate rejected");
    }
    session.disconnect_client(icss::core::ClientRole::TacticalViewer, "viewer reconnect exercised for resilience evidence");
    session.connect_client(icss::core::ClientRole::TacticalViewer, 201U);
    if (!session.issue_command().accepted) {
        throw std::runtime_error("baseline command_issue rejected");
    }
    for (int i = 0; i < config.scenario.engagement_timeout_ticks && !session.latest_snapshot().judgment.ready; ++i) {
        session.advance_tick();
    }
}

}  // namespace

BaselineRuntime::BaselineRuntime(RuntimeConfig config)
    : config_(std::move(config)) {}

const RuntimeConfig& BaselineRuntime::config() const {
    return config_;
}

RuntimeResult BaselineRuntime::run() const {
    auto transport = icss::net::make_transport(icss::net::BackendKind::InProcess, config_);
    drive_baseline_transport_sequence(*transport, config_);
    transport->archive_session();

    const auto summary = transport->summary();
    const auto aar_dir = config_.repo_root / config_.logging.aar_output_dir;
    const auto example_output = config_.repo_root / "examples/sample-output.md";
    const auto log_file = config_.repo_root / config_.logging.file_path;

    transport->write_aar_artifacts(aar_dir);
    transport->write_example_output(example_output, icss::view::make_replay_cursor(summary.event_count, summary.event_count == 0 ? 0 : summary.event_count - 1));

    write_runtime_session_log(config_, transport->backend_name(), summary, transport->events());

    return {config_, summary};
}

RuntimeConfig default_runtime_config(const std::filesystem::path& repo_root) {
    return load_runtime_config(repo_root);
}

SimulationSession run_baseline_demo(const RuntimeConfig& config) {
    SimulationSession session(1001, config.server.tick_rate_hz, config.scenario.telemetry_interval_ms, config.scenario);
    drive_baseline_session_sequence(session, config);
    session.archive_session();
    return session;
}

SimulationSession run_baseline_demo() {
    RuntimeConfig config;
    SimulationSession session(1001, config.server.tick_rate_hz, config.scenario.telemetry_interval_ms, config.scenario);
    drive_baseline_session_sequence(session, config);
    session.archive_session();
    return session;
}

void write_runtime_session_log(const RuntimeConfig& config,
                               std::string_view backend_name,
                               const SessionSummary& summary,
                               const std::vector<EventRecord>& events) {
    const auto log_file = config.repo_root / config.logging.file_path;
    std::filesystem::create_directories(log_file.parent_path());
    std::ofstream log(log_file);
    log << "{\"schema_version\":\"" << kRuntimeLogSchemaVersion
        << "\",\"record_type\":\"session_summary\",\"level\":\"" << escape_json(config.logging.level)
        << "\",\"backend\":\"" << escape_json(backend_name)
        << "\",\"session_id\":" << summary.session_id
        << ",\"phase\":\"" << escape_json(to_string(summary.phase))
        << "\",\"snapshot_count\":" << summary.snapshot_count
        << ",\"event_count\":" << summary.event_count
        << ",\"command_console_connection\":\"" << escape_json(to_string(summary.command_console_connection)) << "\""
        << ",\"viewer_connection\":\"" << escape_json(to_string(summary.viewer_connection)) << "\""
        << ",\"judgment_code\":\"" << escape_json(to_string(summary.judgment_code)) << "\""
        << ",\"last_event_type\":\"" << escape_json(summary.has_last_event ? std::string(icss::protocol::to_string(summary.last_event_type)) : std::string("none")) << "\""
        << ",\"resilience\":\"" << escape_json(summary.resilience_case) << "\"}\n";
    for (const auto& event : events) {
        log << "{\"schema_version\":\"" << kRuntimeLogSchemaVersion << "\",\"record_type\":\"event\""
            << ",\"tick\":" << event.header.tick
            << ",\"timestamp_ms\":" << event.header.timestamp_ms
            << ",\"event_type\":\"" << icss::protocol::to_string(event.header.event_type) << "\""
            << ",\"source\":\"" << escape_json(event.source) << "\""
            << ",\"summary\":\"" << escape_json(event.summary) << "\""
            << ",\"details\":\"" << escape_json(event.details) << "\""
            << ",\"entity_ids\":[";
        for (std::size_t index = 0; index < event.entity_ids.size(); ++index) {
            log << "\"" << escape_json(event.entity_ids[index]) << "\"";
            if (index + 1 != event.entity_ids.size()) {
                log << ",";
            }
        }
        log << "]}\n";
    }
}

}  // namespace icss::core
