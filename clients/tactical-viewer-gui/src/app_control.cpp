#include "app.hpp"

#include <algorithm>
#include <stdexcept>

#include "icss/protocol/serialization.hpp"

namespace icss::viewer_gui {

#if !defined(_WIN32)
void perform_control_action(std::string_view action_label,
                            ViewerState& state,
                            const ViewerOptions& options,
                            FrameMode frame_mode,
                            TcpSocket& control_socket) {
    if (is_profile_button(action_label)) {
        state.profile_label = std::string(action_label);
        state.planned_scenario = scenario_profile(action_label);
        sync_preview_from_planned_scenario(state);
        state.control.last_ok = true;
        state.control.last_label = std::string(action_label);
        state.control.last_message = "scenario profile selected";
        push_timeline_entry(state, "[profile] " + std::string(action_label) + " selected");
        return;
    }

    ensure_control_connected(control_socket, state, options, frame_mode);
    const auto send_and_expect_ack = [&](std::string_view kind, std::string payload) {
        send_frame(control_socket, frame_mode, kind, payload);
        const auto frame = recv_frame(control_socket, frame_mode);
        if (frame.kind != "command_ack") {
            throw std::runtime_error("expected command_ack after " + std::string(kind));
        }
        const auto ack = icss::protocol::parse_command_ack(frame.payload);
        state.control.last_ok = ack.accepted;
        state.control.last_label = std::string(action_label);
        state.control.last_message = ack.reason;
        push_timeline_entry(state, control_timeline_message(action_label, ack.accepted, ack.reason));
        if (!ack.accepted) {
            return;
        }
        if (action_label != "Review") {
            state.review.visible = false;
        }
        if (action_label == "Start") {
            state.snapshot.phase = icss::core::SessionPhase::Detecting;
            state.snapshot.target.active = true;
            state.snapshot.command_status = icss::core::CommandLifecycle::None;
            state.snapshot.asset_status = icss::core::AssetStatus::Idle;
            state.command_visual_active = false;
        } else if (action_label == "Activate") {
            state.snapshot.phase = icss::core::SessionPhase::AssetReady;
            state.snapshot.asset.active = true;
            state.snapshot.asset_status = icss::core::AssetStatus::Ready;
        } else if (action_label == "Track") {
            state.snapshot.phase = icss::core::SessionPhase::Tracking;
            state.snapshot.track.active = true;
            state.snapshot.track.confidence_pct = std::max(state.snapshot.track.confidence_pct, 82);
        } else if (action_label == "Command") {
            state.snapshot.phase = icss::core::SessionPhase::CommandIssued;
            state.snapshot.command_status = icss::core::CommandLifecycle::Accepted;
            state.snapshot.asset_status = icss::core::AssetStatus::Engaging;
            state.command_visual_active = true;
        } else if (action_label == "Stop") {
            state.snapshot.phase = icss::core::SessionPhase::Archived;
        } else if (action_label == "Reset") {
            state.snapshot.phase = icss::core::SessionPhase::Initialized;
            state.snapshot.target.active = false;
            state.snapshot.asset.active = false;
            state.snapshot.track.active = false;
            state.snapshot.track.confidence_pct = 0;
            state.snapshot.command_status = icss::core::CommandLifecycle::None;
            state.snapshot.asset_status = icss::core::AssetStatus::Idle;
            state.snapshot.judgment = {};
            state.command_visual_active = false;
            state.review = {};
        }
    };

    if (action_label == "Start") {
        send_and_expect_ack(
            "scenario_start",
            icss::protocol::serialize(icss::protocol::ScenarioStartPayload{
                {options.session_id, options.sender_id, state.control.sequence++},
                "basic_intercept_training",
                state.planned_scenario.world_width,
                state.planned_scenario.world_height,
                state.planned_scenario.target_start_x,
                state.planned_scenario.target_start_y,
                state.planned_scenario.target_velocity_x,
                state.planned_scenario.target_velocity_y,
                state.planned_scenario.interceptor_start_x,
                state.planned_scenario.interceptor_start_y,
                state.planned_scenario.interceptor_speed_per_tick,
                state.planned_scenario.intercept_radius,
                state.planned_scenario.engagement_timeout_ticks,
                state.planned_scenario.seeker_fov_deg}));
        return;
    }
    if (action_label == "Track") {
        send_and_expect_ack("track_request", icss::protocol::serialize(icss::protocol::TrackRequestPayload{{options.session_id, options.sender_id, state.control.sequence++}, "target-alpha"}));
        return;
    }
    if (action_label == "Activate") {
        send_and_expect_ack("asset_activate", icss::protocol::serialize(icss::protocol::AssetActivatePayload{{options.session_id, options.sender_id, state.control.sequence++}, "asset-interceptor"}));
        return;
    }
    if (action_label == "Command") {
        send_and_expect_ack("command_issue", icss::protocol::serialize(icss::protocol::CommandIssuePayload{{options.session_id, options.sender_id, state.control.sequence++}, "asset-interceptor", "target-alpha"}));
        return;
    }
    if (action_label == "Stop") {
        send_and_expect_ack("scenario_stop", icss::protocol::serialize(icss::protocol::ScenarioStopPayload{{options.session_id, options.sender_id, state.control.sequence++}, "gui control panel stop"}));
        return;
    }
    if (action_label == "Reset") {
        send_and_expect_ack("scenario_reset", icss::protocol::serialize(icss::protocol::ScenarioResetPayload{{options.session_id, options.sender_id, state.control.sequence++}, "gui control panel reset"}));
        state.recent_server_events.clear();
        state.target_history.clear();
        state.asset_history.clear();
        state.last_server_event_tick = 0;
        state.last_server_event_type = "none";
        state.last_server_event_summary = "no server event";
        state.review.visible = false;
        return;
    }
    if (action_label == "Review") {
        if (!review_available(state)) {
            state.control.last_ok = false;
            state.control.last_label = "Review";
            state.control.last_message = "review available after judgment or archive";
            push_timeline_entry(state, control_timeline_message("Review", false, state.control.last_message));
            return;
        }
        send_frame(control_socket,
                   frame_mode,
                   "aar_request",
                   icss::protocol::serialize(icss::protocol::AarRequestPayload{{options.session_id, options.sender_id, state.control.sequence++}, 999U, "absolute"}));
        const auto frame = recv_frame(control_socket, frame_mode);
        if (frame.kind != "aar_response") {
            throw std::runtime_error("expected aar_response after Review action");
        }
        const auto response = icss::protocol::parse_aar_response(frame.payload);
        state.control.last_ok = true;
        state.control.last_label = "Review";
        state.control.last_message = "server-side review loaded";
        push_timeline_entry(state, control_timeline_message("Review", true, state.control.last_message));
        state.review.available = true;
        state.review.loaded = true;
        state.review.visible = true;
        state.review.cursor_index = response.replay_cursor_index;
        state.review.total_events = response.total_events;
        state.review.judgment_code = response.judgment_code;
        state.review.resilience_case = response.resilience_case;
        state.review.event_type = response.event_type;
        state.review.event_summary = response.event_summary;
    }
}
#endif

}  // namespace icss::viewer_gui
