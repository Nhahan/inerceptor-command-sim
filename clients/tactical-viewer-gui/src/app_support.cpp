#include "app.hpp"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <iomanip>
#include <sstream>
#include <stdexcept>

#include "icss/view/ascii_tactical_view.hpp"

namespace icss::viewer_gui {
namespace {

std::filesystem::path resolve_repo_root(std::filesystem::path path) {
    if (std::filesystem::exists(path / "configs/server.example.yaml")) {
        return path;
    }
    if (std::filesystem::exists(path.parent_path() / "configs/server.example.yaml")) {
        return path.parent_path();
    }
    return path;
}

std::filesystem::path default_font_path() {
    const std::vector<std::filesystem::path> candidates {
        "/System/Library/Fonts/Menlo.ttc",
        "/System/Library/Fonts/Supplemental/Courier New.ttf",
        "/System/Library/Fonts/Supplemental/Arial.ttf",
        "/usr/share/fonts/truetype/dejavu/DejaVuSansMono.ttf",
    };
    for (const auto& candidate : candidates) {
        if (std::filesystem::exists(candidate)) {
            return candidate;
        }
    }
    throw std::runtime_error("no usable monospace font found; pass --font PATH");
}

std::vector<std::string> split_controls(const std::string& value) {
    std::vector<std::string> items;
    std::string current;
    for (char ch : value) {
        if (ch == ',') {
            if (!current.empty()) {
                items.push_back(current);
                current.clear();
            }
            continue;
        }
        current.push_back(ch);
    }
    if (!current.empty()) {
        items.push_back(current);
    }
    return items;
}

}  // namespace

void push_timeline_entry(ViewerState& state, std::string message) {
    if (!state.recent_server_events.empty() && state.recent_server_events.front() == message) {
        return;
    }
    state.recent_server_events.push_front(std::move(message));
    while (state.recent_server_events.size() > 16) {
        state.recent_server_events.pop_back();
    }
}

std::string escape_json(std::string_view input) {
    std::string escaped;
    escaped.reserve(input.size());
    for (char ch : input) {
        switch (ch) {
        case '\\': escaped += "\\\\"; break;
        case '"': escaped += "\\\""; break;
        case '\n': escaped += "\\n"; break;
        case '\r': escaped += "\\r"; break;
        case '\t': escaped += "\\t"; break;
        default: escaped += ch; break;
        }
    }
    return escaped;
}

ViewerOptions parse_args(int argc, char** argv) {
    ViewerOptions options;
    for (int i = 1; i < argc; ++i) {
        const std::string_view arg = argv[i];
        auto require_value = [&](std::string_view label) -> std::string {
            if (i + 1 >= argc) {
                throw std::runtime_error("missing value for " + std::string(label));
            }
            return argv[++i];
        };
        if (arg == "--host") {
            options.host = require_value(arg);
            continue;
        }
        if (arg == "--udp-port") {
            options.udp_port = static_cast<std::uint16_t>(std::stoul(require_value(arg)));
            continue;
        }
        if (arg == "--tcp-port") {
            options.tcp_port = static_cast<std::uint16_t>(std::stoul(require_value(arg)));
            continue;
        }
        if (arg == "--session-id") {
            options.session_id = static_cast<std::uint32_t>(std::stoul(require_value(arg)));
            continue;
        }
        if (arg == "--sender-id") {
            options.sender_id = static_cast<std::uint32_t>(std::stoul(require_value(arg)));
            continue;
        }
        if (arg == "--duration-ms") {
            options.duration_ms = std::stoull(require_value(arg));
            continue;
        }
        if (arg == "--heartbeat-interval-ms") {
            options.heartbeat_interval_ms = std::stoull(require_value(arg));
            continue;
        }
        if (arg == "--repo-root") {
            options.repo_root = require_value(arg);
            continue;
        }
        if (arg == "--tcp-frame-format") {
            options.tcp_frame_format = require_value(arg);
            continue;
        }
        if (arg == "--auto-controls") {
            options.auto_controls = split_controls(require_value(arg));
            options.auto_control_script = true;
            continue;
        }
        if (arg == "--width") {
            options.width = std::stoi(require_value(arg));
            continue;
        }
        if (arg == "--height") {
            options.height = std::stoi(require_value(arg));
            continue;
        }
        if (arg == "--dump-state") {
            options.dump_state_path = require_value(arg);
            continue;
        }
        if (arg == "--dump-frame") {
            options.dump_frame_path = require_value(arg);
            continue;
        }
        if (arg == "--font") {
            options.font_path = require_value(arg);
            continue;
        }
        if (arg == "--hidden") {
            options.hidden = true;
            continue;
        }
        if (arg == "--headless") {
            options.hidden = true;
            options.headless = true;
            continue;
        }
        if (arg == "--auto-control-script") {
            options.auto_control_script = true;
            continue;
        }
        if (arg == "--help") {
            std::printf(
                "usage: icss_tactical_viewer_gui [--host HOST] [--udp-port PORT] [--tcp-port PORT]\n"
                "       [--session-id ID] [--sender-id ID] [--duration-ms MS] [--heartbeat-interval-ms MS] [--repo-root PATH]\n"
                "       [--tcp-frame-format json|binary] [--width PX] [--height PX]\n"
                "       [--dump-state PATH] [--dump-frame PATH] [--font PATH] [--hidden] [--headless]\n"
                "       [--auto-control-script] [--auto-controls Start,Track,...]\n");
            std::exit(0);
        }
        throw std::runtime_error("unknown argument: " + std::string(arg));
    }
    if (options.font_path.empty()) {
        options.font_path = default_font_path();
    }
    return options;
}

icss::core::AssetStatus parse_asset_status(std::string_view value) {
    if (value == "idle") return icss::core::AssetStatus::Idle;
    if (value == "ready") return icss::core::AssetStatus::Ready;
    if (value == "engaging") return icss::core::AssetStatus::Engaging;
    if (value == "complete") return icss::core::AssetStatus::Complete;
    return icss::core::AssetStatus::Idle;
}

icss::core::CommandLifecycle parse_command_status(std::string_view value) {
    if (value == "none") return icss::core::CommandLifecycle::None;
    if (value == "accepted") return icss::core::CommandLifecycle::Accepted;
    if (value == "executing") return icss::core::CommandLifecycle::Executing;
    if (value == "completed") return icss::core::CommandLifecycle::Completed;
    if (value == "rejected") return icss::core::CommandLifecycle::Rejected;
    return icss::core::CommandLifecycle::None;
}

icss::core::JudgmentCode parse_judgment_code(std::string_view value) {
    if (value == "intercept_success") return icss::core::JudgmentCode::InterceptSuccess;
    if (value == "invalid_transition") return icss::core::JudgmentCode::InvalidTransition;
    if (value == "timeout_observed") return icss::core::JudgmentCode::TimeoutObserved;
    return icss::core::JudgmentCode::Pending;
}

icss::core::ConnectionState parse_connection_state(std::string_view value) {
    if (value == "connected") return icss::core::ConnectionState::Connected;
    if (value == "reconnected") return icss::core::ConnectionState::Reconnected;
    if (value == "timed_out") return icss::core::ConnectionState::TimedOut;
    return icss::core::ConnectionState::Disconnected;
}

icss::core::SessionPhase parse_phase(std::string_view value) {
    if (value == "initialized") return icss::core::SessionPhase::Initialized;
    if (value == "detecting") return icss::core::SessionPhase::Detecting;
    if (value == "tracking") return icss::core::SessionPhase::Tracking;
    if (value == "asset_ready") return icss::core::SessionPhase::AssetReady;
    if (value == "command_issued") return icss::core::SessionPhase::CommandIssued;
    if (value == "engaging") return icss::core::SessionPhase::Engaging;
    if (value == "judged") return icss::core::SessionPhase::Judged;
    if (value == "archived") return icss::core::SessionPhase::Archived;
    return icss::core::SessionPhase::Initialized;
}

std::string uppercase_words(std::string_view value) {
    std::string out;
    out.reserve(value.size());
    for (char ch : value) {
        if (ch == '_') {
            out.push_back(' ');
            continue;
        }
        if (ch >= 'a' && ch <= 'z') {
            out.push_back(static_cast<char>(ch - ('a' - 'A')));
        } else {
            out.push_back(ch);
        }
    }
    return out;
}

std::string phase_banner_label(icss::core::SessionPhase phase) {
    switch (phase) {
    case icss::core::SessionPhase::Initialized: return "INITIALIZED";
    case icss::core::SessionPhase::Detecting: return "DETECTING";
    case icss::core::SessionPhase::Tracking: return "TRACKING";
    case icss::core::SessionPhase::AssetReady: return "INTERCEPTOR READY";
    case icss::core::SessionPhase::CommandIssued: return "COMMAND ACCEPTED";
    case icss::core::SessionPhase::Engaging: return "ENGAGING";
    case icss::core::SessionPhase::Judged: return "JUDGMENT PRODUCED";
    case icss::core::SessionPhase::Archived: return "ARCHIVED";
    }
    return "UNKNOWN";
}

std::string phase_operator_note(icss::core::SessionPhase phase) {
    switch (phase) {
    case icss::core::SessionPhase::Initialized:
        return "Ready for scenario start. No live target track yet.";
    case icss::core::SessionPhase::Detecting:
        return "Target is present. Awaiting track confirmation.";
    case icss::core::SessionPhase::Tracking:
        return "Track lock acquired. Interceptor can be prepared.";
    case icss::core::SessionPhase::AssetReady:
        return "Interceptor ready. Operator can issue command.";
    case icss::core::SessionPhase::CommandIssued:
        return "Command accepted by authoritative server.";
    case icss::core::SessionPhase::Engaging:
        return "Engagement in progress. Awaiting judgment.";
    case icss::core::SessionPhase::Judged:
        return "Authoritative judgment complete. Review result or archive.";
    case icss::core::SessionPhase::Archived:
        return "Session archived. Reset to begin a new run.";
    }
    return "Unknown phase.";
}

SDL_Color phase_accent(icss::core::SessionPhase phase) {
    switch (phase) {
    case icss::core::SessionPhase::Initialized: return SDL_Color {143, 157, 179, 255};
    case icss::core::SessionPhase::Detecting: return SDL_Color {244, 180, 0, 255};
    case icss::core::SessionPhase::Tracking: return SDL_Color {80, 200, 120, 255};
    case icss::core::SessionPhase::AssetReady: return SDL_Color {77, 171, 247, 255};
    case icss::core::SessionPhase::CommandIssued: return SDL_Color {255, 149, 0, 255};
    case icss::core::SessionPhase::Engaging: return SDL_Color {255, 99, 71, 255};
    case icss::core::SessionPhase::Judged: return SDL_Color {179, 136, 255, 255};
    case icss::core::SessionPhase::Archived: return SDL_Color {130, 130, 130, 255};
    }
    return SDL_Color {143, 157, 179, 255};
}

std::string resilience_summary(const ViewerState& state) {
    const auto freshness = icss::view::freshness_label(state.snapshot);
    if (freshness == "resync") {
        return "Viewer reconnected. Latest snapshot is resynchronizing state.";
    }
    if (freshness == "degraded") {
        return "Packet loss observed. Viewer converged using latest valid snapshot.";
    }
    if (freshness == "stale") {
        return "Viewer state is stale. Treat positional data as low confidence.";
    }
    return "Steady-state propagation. Viewer is following authoritative snapshots.";
}

std::string authoritative_headline(const ViewerState& state) {
    if (state.snapshot.judgment.ready) {
        return "JUDGMENT: " + uppercase_words(icss::core::to_string(state.snapshot.judgment.code));
    }
    if (!state.control.last_ok && state.control.last_label != "idle") {
        return "REJECTED CONTROL: " + uppercase_words(state.control.last_label);
    }
    if (state.snapshot.command_status == icss::core::CommandLifecycle::Executing) {
        return "ACTIVE COMMAND: INTERCEPTOR ENGAGING";
    }
    if (state.snapshot.command_status == icss::core::CommandLifecycle::Accepted
        || state.snapshot.phase == icss::core::SessionPhase::CommandIssued) {
        return "COMMAND ACCEPTED: AWAITING LIVE ENGAGEMENT";
    }
    if (!state.recent_server_events.empty()) {
        return "LAST EVENT: " + uppercase_words(state.last_server_event_type);
    }
    return "AWAITING AUTHORITATIVE EVENT";
}

std::string authoritative_badge_label(const ViewerState& state) {
    if (state.snapshot.judgment.ready) {
        return "JUDGED";
    }
    if (!state.control.last_ok && state.control.last_label != "idle") {
        return "REJECTED";
    }
    if (state.snapshot.command_status == icss::core::CommandLifecycle::Executing) {
        return "ENGAGING";
    }
    if (state.snapshot.command_status == icss::core::CommandLifecycle::Accepted
        || state.snapshot.phase == icss::core::SessionPhase::CommandIssued) {
        return "ACCEPTED";
    }
    if (!state.recent_server_events.empty()) {
        return "LIVE";
    }
    return "AWAITING";
}

SDL_Color authoritative_badge_color(const ViewerState& state) {
    if (state.snapshot.judgment.ready) {
        return SDL_Color {179, 136, 255, 255};
    }
    if (!state.control.last_ok && state.control.last_label != "idle") {
        return SDL_Color {255, 99, 120, 255};
    }
    if (state.snapshot.command_status == icss::core::CommandLifecycle::Executing) {
        return SDL_Color {255, 149, 0, 255};
    }
    if (state.snapshot.command_status == icss::core::CommandLifecycle::Accepted
        || state.snapshot.phase == icss::core::SessionPhase::CommandIssued) {
        return SDL_Color {255, 201, 107, 255};
    }
    if (!state.recent_server_events.empty()) {
        return SDL_Color {120, 190, 255, 255};
    }
    return SDL_Color {143, 157, 179, 255};
}

bool review_available(const ViewerState& state) {
    return state.snapshot.phase == icss::core::SessionPhase::Judged
        || state.snapshot.phase == icss::core::SessionPhase::Archived
        || state.snapshot.judgment.ready;
}

std::string review_panel_text(const ViewerState& state) {
    if (!review_available(state)) {
        return "Review unavailable until authoritative judgment or archive.\n"
               "Use the live log during active control.";
    }
    if (!state.review.loaded) {
        return "Server-side review is available.\n"
               "Press Review to load AAR cursor, judgment, resilience, and event summary.";
    }

    std::string text;
    text += "AAR cursor: " + std::to_string(state.review.cursor_index) + "/" + std::to_string(state.review.total_events) + "\n";
    text += "Judgment: " + state.review.judgment_code + "\n";
    text += "Resilience: " + state.review.resilience_case + "\n";
    text += "Reviewed event: " + state.review.event_type + "\n";
    text += "Summary: " + state.review.event_summary;
    return text;
}

std::string terminal_timeline_text(const ViewerState& state, bool review_mode) {
    std::string out;
    out += review_mode ? "mode=review | source=server_aar\n" : "mode=live | source=server_events\n";
    out += "----------------------------------------\n";
    if (review_mode) {
        const auto body = review_panel_text(state);
        std::size_t start = 0;
        while (start <= body.size()) {
            const auto end = body.find('\n', start);
            const auto line = body.substr(start, end == std::string::npos ? std::string::npos : end - start);
            out += "> " + line + "\n";
            if (end == std::string::npos) {
                break;
            }
            start = end + 1;
        }
        return out;
    }

    if (state.recent_server_events.empty()) {
        out += "> waiting for authoritative telemetry or control activity\n";
        return out;
    }
    for (const auto& event : state.recent_server_events) {
        out += "> " + event + "\n";
    }
    return out;
}

std::string control_timeline_message(std::string_view label, bool ok, std::string_view message) {
    return std::string(ok ? "[control accepted] " : "[control rejected] ")
        + std::string(label) + " | " + std::string(message);
}

std::string latest_timeline_entry(const ViewerState& state) {
    if (state.recent_server_events.empty()) {
        return "no timeline activity yet";
    }
    return state.recent_server_events.front();
}

std::string telemetry_event_status(const ViewerState& state) {
    if (state.last_server_event_type == "none") {
        return "pending";
    }
    return "[tick " + std::to_string(state.last_server_event_tick) + "] "
        + state.last_server_event_summary + " (" + state.last_server_event_type + ")";
}

std::string format_fixed_1(float value) {
    std::ostringstream out;
    out << std::fixed << std::setprecision(1) << value;
    return out.str();
}

std::string format_fixed_0(std::uint32_t value) {
    return std::to_string(value);
}

float heading_deg_gui(icss::core::Vec2f v) {
    if (std::abs(v.x) <= 1e-5F && std::abs(v.y) <= 1e-5F) {
        return 0.0F;
    }
    return std::atan2(v.y, v.x) * 180.0F / 3.1415926535F;
}

icss::core::ScenarioConfig default_viewer_scenario(const std::filesystem::path& repo_root) {
    try {
        return icss::core::load_runtime_config(resolve_repo_root(repo_root)).scenario;
    } catch (const std::exception&) {
        return icss::core::ScenarioConfig {};
    }
}

namespace {

int clamp_int(int value, int lo, int hi) {
    return std::max(lo, std::min(value, hi));
}

std::string setup_message(std::string_view field, int value) {
    return std::string("next start ") + std::string(field) + " = " + std::to_string(value);
}

}  // namespace

bool apply_parameter_action(ViewerState& state, std::string_view action) {
    auto& scenario = state.planned_scenario;
    auto update_preview = [&](std::string_view field, int value) {
        const bool preview_allowed = !state.received_snapshot
            || state.snapshot.phase == icss::core::SessionPhase::Initialized;
        if (preview_allowed) {
            sync_preview_from_planned_scenario(state);
        }
        state.control.last_ok = true;
        state.control.last_label = "Setup";
        state.control.last_message = setup_message(field, value);
        state.review.visible = false;
        push_timeline_entry(state, "[setup] " + std::string(field) + " -> " + std::to_string(value));
    };

    if (action == "target_pos_x_dec") {
        scenario.target_start_x = clamp_int(scenario.target_start_x - 16, 0, scenario.world_width - 1);
        update_preview("target_start_x", scenario.target_start_x);
        return true;
    }
    if (action == "target_pos_x_inc") {
        scenario.target_start_x = clamp_int(scenario.target_start_x + 16, 0, scenario.world_width - 1);
        update_preview("target_start_x", scenario.target_start_x);
        return true;
    }
    if (action == "target_pos_y_dec") {
        scenario.target_start_y = clamp_int(scenario.target_start_y - 16, 0, scenario.world_height - 1);
        update_preview("target_start_y", scenario.target_start_y);
        return true;
    }
    if (action == "target_pos_y_inc") {
        scenario.target_start_y = clamp_int(scenario.target_start_y + 16, 0, scenario.world_height - 1);
        update_preview("target_start_y", scenario.target_start_y);
        return true;
    }
    if (action == "target_vel_x_dec") {
        scenario.target_velocity_x = clamp_int(scenario.target_velocity_x - 1, -12, 12);
        update_preview("target_velocity_x", scenario.target_velocity_x);
        return true;
    }
    if (action == "target_vel_x_inc") {
        scenario.target_velocity_x = clamp_int(scenario.target_velocity_x + 1, -12, 12);
        update_preview("target_velocity_x", scenario.target_velocity_x);
        return true;
    }
    if (action == "target_vel_y_dec") {
        scenario.target_velocity_y = clamp_int(scenario.target_velocity_y - 1, -12, 12);
        update_preview("target_velocity_y", scenario.target_velocity_y);
        return true;
    }
    if (action == "target_vel_y_inc") {
        scenario.target_velocity_y = clamp_int(scenario.target_velocity_y + 1, -12, 12);
        update_preview("target_velocity_y", scenario.target_velocity_y);
        return true;
    }
    if (action == "asset_pos_x_dec") {
        scenario.interceptor_start_x = clamp_int(scenario.interceptor_start_x - 16, 0, scenario.world_width - 1);
        update_preview("interceptor_start_x", scenario.interceptor_start_x);
        return true;
    }
    if (action == "asset_pos_x_inc") {
        scenario.interceptor_start_x = clamp_int(scenario.interceptor_start_x + 16, 0, scenario.world_width - 1);
        update_preview("interceptor_start_x", scenario.interceptor_start_x);
        return true;
    }
    if (action == "asset_pos_y_dec") {
        scenario.interceptor_start_y = clamp_int(scenario.interceptor_start_y - 16, 0, scenario.world_height - 1);
        update_preview("interceptor_start_y", scenario.interceptor_start_y);
        return true;
    }
    if (action == "asset_pos_y_inc") {
        scenario.interceptor_start_y = clamp_int(scenario.interceptor_start_y + 16, 0, scenario.world_height - 1);
        update_preview("interceptor_start_y", scenario.interceptor_start_y);
        return true;
    }
    if (action == "asset_speed_dec") {
        scenario.interceptor_speed_per_tick = clamp_int(scenario.interceptor_speed_per_tick - 8, 4, 96);
        update_preview("interceptor_speed_per_tick", scenario.interceptor_speed_per_tick);
        return true;
    }
    if (action == "asset_speed_inc") {
        scenario.interceptor_speed_per_tick = clamp_int(scenario.interceptor_speed_per_tick + 8, 4, 96);
        update_preview("interceptor_speed_per_tick", scenario.interceptor_speed_per_tick);
        return true;
    }
    if (action == "timeout_dec") {
        scenario.engagement_timeout_ticks = clamp_int(scenario.engagement_timeout_ticks - 10, 10, 180);
        update_preview("engagement_timeout_ticks", scenario.engagement_timeout_ticks);
        return true;
    }
    if (action == "timeout_inc") {
        scenario.engagement_timeout_ticks = clamp_int(scenario.engagement_timeout_ticks + 10, 10, 180);
        update_preview("engagement_timeout_ticks", scenario.engagement_timeout_ticks);
        return true;
    }
    return false;
}

bool is_live_control_action(std::string_view action) {
    return action == "Start"
        || action == "Track"
        || action == "Activate"
        || action == "Command"
        || action == "Stop"
        || action == "Reset"
        || action == "Review";
}

std::string recommended_control_label(const ViewerState& state) {
    switch (state.snapshot.phase) {
    case icss::core::SessionPhase::Initialized:
        return "Start";
    case icss::core::SessionPhase::Detecting:
        return "Track";
    case icss::core::SessionPhase::Tracking:
        return "Activate";
    case icss::core::SessionPhase::AssetReady:
        return "Command";
    case icss::core::SessionPhase::CommandIssued:
    case icss::core::SessionPhase::Engaging:
    case icss::core::SessionPhase::Judged:
        return "Stop";
    case icss::core::SessionPhase::Archived:
        if (review_available(state) && !state.review.loaded) {
            return "Review";
        }
        return "Reset";
    }
    return "";
}

bool target_motion_visual_visible(const ViewerState& state) {
    return state.snapshot.track.active
        && state.target_history.size() > 1
        && std::sqrt(state.snapshot.target_velocity.x * state.snapshot.target_velocity.x
            + state.snapshot.target_velocity.y * state.snapshot.target_velocity.y) > 0.1F;
}

bool asset_motion_visual_visible(const ViewerState& state) {
    return state.snapshot.asset.active
        && state.asset_history.size() > 1
        && std::sqrt(state.snapshot.asset_velocity.x * state.snapshot.asset_velocity.x
            + state.snapshot.asset_velocity.y * state.snapshot.asset_velocity.y) > 0.1F;
}

bool engagement_visual_visible(const ViewerState& state) {
    return state.snapshot.target.active
        && state.snapshot.asset.active
        && state.snapshot.predicted_intercept_valid
        && state.target_history.size() > 1
        && state.asset_history.size() > 1
        && (state.snapshot.phase == icss::core::SessionPhase::Engaging
            || state.snapshot.command_status == icss::core::CommandLifecycle::Executing
            || state.snapshot.command_status == icss::core::CommandLifecycle::Completed
            || state.snapshot.judgment.ready);
}

bool predicted_marker_visual_visible(const ViewerState& state) {
    return state.snapshot.predicted_intercept_valid;
}

bool command_visual_active(const ViewerState& state) {
    return state.snapshot.command_status == icss::core::CommandLifecycle::Executing
        || state.snapshot.command_status == icss::core::CommandLifecycle::Completed
        || state.snapshot.judgment.ready;
}

void sync_preview_from_planned_scenario(ViewerState& state) {
    state.snapshot.world_width = state.planned_scenario.world_width;
    state.snapshot.world_height = state.planned_scenario.world_height;
    state.snapshot.target.position = {state.planned_scenario.target_start_x, state.planned_scenario.target_start_y};
    state.snapshot.asset.position = {state.planned_scenario.interceptor_start_x, state.planned_scenario.interceptor_start_y};
    state.snapshot.target_world_position = {static_cast<float>(state.planned_scenario.target_start_x),
                                            static_cast<float>(state.planned_scenario.target_start_y)};
    state.snapshot.asset_world_position = {static_cast<float>(state.planned_scenario.interceptor_start_x),
                                           static_cast<float>(state.planned_scenario.interceptor_start_y)};
    state.snapshot.target_velocity_x = state.planned_scenario.target_velocity_x;
    state.snapshot.target_velocity_y = state.planned_scenario.target_velocity_y;
    state.snapshot.target_velocity = {static_cast<float>(state.planned_scenario.target_velocity_x),
                                      static_cast<float>(state.planned_scenario.target_velocity_y)};
    state.snapshot.asset_velocity = {0.0F, 0.0F};
    state.snapshot.target_heading_deg = heading_deg_gui(state.snapshot.target_velocity);
    state.snapshot.asset_heading_deg = 0.0F;
    state.snapshot.interceptor_speed_per_tick = state.planned_scenario.interceptor_speed_per_tick;
    state.snapshot.interceptor_acceleration_per_tick =
        std::max(1.0F, static_cast<float>(state.planned_scenario.interceptor_speed_per_tick) * 0.28F);
    state.snapshot.intercept_radius = state.planned_scenario.intercept_radius;
    state.snapshot.engagement_timeout_ticks = state.planned_scenario.engagement_timeout_ticks;
    state.snapshot.seeker_fov_deg = static_cast<float>(state.planned_scenario.seeker_fov_deg);
    state.snapshot.seeker_lock = false;
    state.snapshot.off_boresight_deg = 0.0F;
    state.snapshot.predicted_intercept_valid = false;
    state.snapshot.predicted_intercept_position = {};
    state.snapshot.time_to_intercept_s = 0.0F;
    state.snapshot.track = {};
}

void apply_snapshot(ViewerState& state, const icss::protocol::SnapshotPayload& payload) {
    state.snapshot.envelope = payload.envelope;
    state.snapshot.header = payload.header;
    state.snapshot.phase = parse_phase(payload.phase);
    state.snapshot.world_width = payload.world_width;
    state.snapshot.world_height = payload.world_height;
    state.snapshot.target.id = payload.target_id;
    state.snapshot.target.active = payload.target_active;
    state.snapshot.target.position = {payload.target_x, payload.target_y};
    state.snapshot.target_world_position = {payload.target_world_x, payload.target_world_y};
    state.snapshot.target_velocity_x = payload.target_velocity_x;
    state.snapshot.target_velocity_y = payload.target_velocity_y;
    state.snapshot.target_velocity = {payload.target_velocity_world_x, payload.target_velocity_world_y};
    state.snapshot.target_heading_deg = payload.target_heading_deg;
    state.snapshot.asset.id = payload.asset_id;
    state.snapshot.asset.active = payload.asset_active;
    state.snapshot.asset.position = {payload.asset_x, payload.asset_y};
    state.snapshot.asset_world_position = {payload.asset_world_x, payload.asset_world_y};
    state.snapshot.asset_velocity = {payload.asset_velocity_world_x, payload.asset_velocity_world_y};
    state.snapshot.asset_heading_deg = payload.asset_heading_deg;
    state.snapshot.interceptor_speed_per_tick = payload.interceptor_speed_per_tick;
    state.snapshot.interceptor_acceleration_per_tick = payload.interceptor_acceleration_per_tick;
    state.snapshot.intercept_radius = payload.intercept_radius;
    state.snapshot.engagement_timeout_ticks = payload.engagement_timeout_ticks;
    state.snapshot.seeker_fov_deg = payload.seeker_fov_deg;
    state.snapshot.seeker_lock = payload.seeker_lock;
    state.snapshot.off_boresight_deg = payload.off_boresight_deg;
    state.snapshot.predicted_intercept_valid = payload.predicted_intercept_valid;
    state.snapshot.predicted_intercept_position = {payload.predicted_intercept_x, payload.predicted_intercept_y};
    state.snapshot.time_to_intercept_s = payload.time_to_intercept_s;
    state.snapshot.track.active = payload.tracking_active;
    state.snapshot.track.confidence_pct = payload.track_confidence_pct;
    state.snapshot.track.estimated_position = {payload.track_estimated_x, payload.track_estimated_y};
    state.snapshot.track.estimated_velocity = {payload.track_estimated_vx, payload.track_estimated_vy};
    state.snapshot.track.measurement_valid = payload.track_measurement_valid;
    state.snapshot.track.measurement_position = {payload.track_measurement_x, payload.track_measurement_y};
    state.snapshot.track.covariance_trace = payload.track_covariance_trace;
    state.snapshot.track.measurement_age_ticks = static_cast<std::uint32_t>(std::max(payload.track_measurement_age_ticks, 0));
    state.snapshot.track.missed_updates = static_cast<std::uint32_t>(std::max(payload.track_missed_updates, 0));
    state.snapshot.asset_status = parse_asset_status(payload.asset_status);
    state.snapshot.command_status = parse_command_status(payload.command_status);
    state.snapshot.judgment.ready = payload.judgment_ready;
    state.snapshot.judgment.code = parse_judgment_code(payload.judgment_code);
    state.review.available = review_available(state);
    state.received_snapshot = true;
    ++state.snapshot_count_received;
    const auto target_history_point = icss::core::Vec2 {
        static_cast<int>(std::lround(state.snapshot.target_world_position.x)),
        static_cast<int>(std::lround(state.snapshot.target_world_position.y)),
    };
    const auto asset_history_point = icss::core::Vec2 {
        static_cast<int>(std::lround(state.snapshot.asset_world_position.x)),
        static_cast<int>(std::lround(state.snapshot.asset_world_position.y)),
    };
    if (state.target_history.empty() || state.target_history.back().x != target_history_point.x
        || state.target_history.back().y != target_history_point.y) {
        state.target_history.push_back(target_history_point);
        while (state.target_history.size() > 24) {
            state.target_history.pop_front();
        }
    }
    if (state.asset_history.empty() || state.asset_history.back().x != asset_history_point.x
        || state.asset_history.back().y != asset_history_point.y) {
        state.asset_history.push_back(asset_history_point);
        while (state.asset_history.size() > 24) {
            state.asset_history.pop_front();
        }
    }
    if (state.snapshot.phase == icss::core::SessionPhase::Initialized) {
        state.target_history.clear();
        state.asset_history.clear();
    }
}

void apply_telemetry(ViewerState& state, const icss::protocol::TelemetryPayload& payload) {
    state.snapshot.telemetry = payload.sample;
    state.snapshot.viewer_connection = parse_connection_state(payload.connection_state);
    state.received_telemetry = true;
    ++state.telemetry_count_received;
    if (payload.event_type != "none"
        && (payload.event_tick != state.last_server_event_tick
            || payload.event_type != state.last_server_event_type
            || payload.event_summary != state.last_server_event_summary)) {
        state.last_server_event_tick = payload.event_tick;
        state.last_server_event_type = payload.event_type;
        state.last_server_event_summary = payload.event_summary;
        push_timeline_entry(
            state,
            "[tick " + std::to_string(payload.event_tick) + "] " + payload.event_summary + " (" + payload.event_type + ")");
    }
}

}  // namespace icss::viewer_gui
