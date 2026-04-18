#include "render_internal.hpp"

#include <array>

#include "icss/view/ascii_tactical_view.hpp"

namespace icss::viewer_gui {

void render_phase_panel(SDL_Renderer* renderer, const RenderContext& ctx) {
    const auto& panel = ctx.layout.phase_panel;
    fill_panel(renderer, panel, rgba(12, 14, 20), rgba(54, 60, 78));
    draw_text(renderer, ctx.title_font, panel.x + 12, panel.y + 10, rgba(141, 211, 199), "Mission Phase");
    const std::array<std::pair<icss::core::SessionPhase, const char*>, 8> phase_flow {{
        {icss::core::SessionPhase::Initialized, "INITIALIZED"},
        {icss::core::SessionPhase::Detecting, "DETECTING"},
        {icss::core::SessionPhase::Tracking, "TRACKING"},
        {icss::core::SessionPhase::AssetReady, "INTERCEPTOR READY"},
        {icss::core::SessionPhase::CommandIssued, "COMMAND ACCEPTED"},
        {icss::core::SessionPhase::Engaging, "ENGAGING"},
        {icss::core::SessionPhase::Judged, "JUDGMENT PRODUCED"},
        {icss::core::SessionPhase::Archived, "ARCHIVED"},
    }};
    for (std::size_t index = 0; index < phase_flow.size(); ++index) {
        const SDL_Rect row {panel.x + 12, panel.y + 42 + static_cast<int>(index) * 20, panel.w - 24, 18};
        const auto row_phase = phase_flow[index].first;
        const auto active = row_phase == ctx.state.snapshot.phase;
        const auto completed = static_cast<int>(row_phase) < static_cast<int>(ctx.state.snapshot.phase);
        if (active) {
            SDL_SetRenderDrawColor(renderer, ctx.phase_color.r, ctx.phase_color.g, ctx.phase_color.b, 255);
            SDL_RenderFillRect(renderer, &row);
            draw_text(renderer, ctx.body_font, row.x + 8, row.y + 1, rgba(10, 12, 16), phase_flow[index].second);
        } else {
            SDL_SetRenderDrawColor(renderer, completed ? 68 : 42, completed ? 91 : 50, completed ? 72 : 58, 255);
            SDL_RenderFillRect(renderer, &row);
            SDL_SetRenderDrawColor(renderer, 66, 74, 92, 255);
            SDL_RenderDrawRect(renderer, &row);
            draw_text(renderer,
                      ctx.body_font,
                      row.x + 8,
                      row.y + 1,
                      completed ? rgba(176, 216, 184) : rgba(148, 156, 172),
                      phase_flow[index].second);
        }
    }
}

void render_decision_panel(SDL_Renderer* renderer, const RenderContext& ctx) {
    const auto& panel = ctx.layout.decision_panel;
    fill_panel(renderer, panel, rgba(12, 14, 20), rgba(54, 60, 78));
    draw_text(renderer, ctx.title_font, panel.x + 12, panel.y + 10, rgba(255, 179, 102), "Authoritative Status");
    std::string block;
    block += "Last telemetry event: " + telemetry_event_status(ctx.state) + "\n";
    block += "Latest timeline entry: " + latest_timeline_entry(ctx.state) + "\n";
    block += std::string(ctx.state.control.last_ok ? "Last accepted control: " : "Last rejected control: ")
        + ctx.state.control.last_label + " | " + ctx.state.control.last_message + "\n";
    block += "Judgment: " + std::string(icss::core::to_string(ctx.state.snapshot.judgment.code))
        + " | Command lifecycle: " + std::string(icss::core::to_string(ctx.state.snapshot.command_status)) + "\n";
    block += "Interceptor status: " + std::string(icss::core::to_string(ctx.state.snapshot.asset_status))
        + " | AAR cursor: live";
    draw_text(renderer, ctx.body_font, panel.x + 12, panel.y + 44, rgba(220, 223, 230), block, panel.w - 24);
}

void render_resilience_panel(SDL_Renderer* renderer, const RenderContext& ctx) {
    const auto& panel = ctx.layout.resilience_panel;
    fill_panel(renderer, panel, rgba(12, 14, 20), rgba(54, 60, 78));
    draw_text(renderer, ctx.title_font, panel.x + 12, panel.y + 10, rgba(129, 199, 255), "Resilience / Telemetry");
    std::string block;
    block += "Connection: " + std::string(icss::core::to_string(ctx.state.snapshot.viewer_connection))
        + " | Freshness: " + icss::view::freshness_label(ctx.state.snapshot) + "\n";
    block += "Tick: " + std::to_string(ctx.state.snapshot.telemetry.tick)
        + " | Snapshot: " + std::to_string(ctx.state.snapshot.header.snapshot_sequence) + "\n";
    block += "Latency: " + format_fixed_0(ctx.state.snapshot.telemetry.latency_ms)
        + " ms | Loss: " + format_fixed_1(ctx.state.snapshot.telemetry.packet_loss_pct) + " %";
    block += ctx.state.snapshot.telemetry.packet_loss_pct > 0.0F ? " | degraded sample" : " | stable";
    block += "\n";
    block += "I/O: snapshots=" + std::to_string(ctx.state.snapshot_count_received)
        + ", telemetry=" + std::to_string(ctx.state.telemetry_count_received)
        + ", heartbeats=" + std::to_string(ctx.state.heartbeat_count) + "\n";
    block += "Note: " + ctx.resilience_note;
    draw_text(renderer, ctx.body_font, panel.x + 12, panel.y + 44, rgba(220, 223, 230), block, panel.w - 24);
}

void render_entity_panel(SDL_Renderer* renderer, const RenderContext& ctx) {
    const auto& panel = ctx.layout.entity_panel;
    fill_panel(renderer, panel, rgba(12, 14, 20), rgba(54, 60, 78));
    draw_text(renderer, ctx.title_font, panel.x + 12, panel.y + 10, rgba(220, 223, 230), "Entity Picture");
    draw_text(renderer, ctx.body_font, panel.x + 170, panel.y + 14, rgba(140, 149, 168), "target=red | interceptor=blue | predicted intercept=green");
    std::string block;
    block += "Target " + ctx.state.snapshot.target.id + " @ (" + format_fixed_1(ctx.state.snapshot.target_world_position.x)
        + ", " + format_fixed_1(ctx.state.snapshot.target_world_position.y) + ") active=" + (ctx.state.snapshot.target.active ? "yes" : "no") + "\n";
    block += "Interceptor " + ctx.state.snapshot.asset.id + " @ (" + format_fixed_1(ctx.state.snapshot.asset_world_position.x)
        + ", " + format_fixed_1(ctx.state.snapshot.asset_world_position.y) + ") active=" + (ctx.state.snapshot.asset.active ? "yes" : "no") + "\n";
    block += "Target v=(" + format_fixed_1(ctx.state.snapshot.target_velocity.x) + "," + format_fixed_1(ctx.state.snapshot.target_velocity.y)
        + ") hdg=" + format_fixed_1(ctx.state.snapshot.target_heading_deg)
        + " | Interceptor v=(" + format_fixed_1(ctx.state.snapshot.asset_velocity.x) + "," + format_fixed_1(ctx.state.snapshot.asset_velocity.y)
        + ") hdg=" + format_fixed_1(ctx.state.snapshot.asset_heading_deg) + "\n";
    block += "Tracking " + std::string(ctx.state.snapshot.track.active ? "LOCKED" : "OFF")
        + " (" + std::to_string(ctx.state.snapshot.track.confidence_pct) + "%)"
        + " | cov=" + format_fixed_1(ctx.state.snapshot.track.covariance_trace)
        + " | age=" + std::to_string(ctx.state.snapshot.track.measurement_age_ticks)
        + " | misses=" + std::to_string(ctx.state.snapshot.track.missed_updates)
        + " | TTI=" + (ctx.state.snapshot.predicted_intercept_valid ? format_fixed_1(ctx.state.snapshot.time_to_intercept_s) + "s" : std::string("pending"))
        + " | seeker_lock=" + (ctx.state.snapshot.predicted_intercept_valid ? (ctx.state.snapshot.seeker_lock ? "yes" : "no") : "n/a") + "\n";
    if (ctx.state.snapshot.predicted_intercept_valid) {
        block += "Predicted intercept=(" + format_fixed_1(ctx.state.snapshot.predicted_intercept_position.x)
            + "," + format_fixed_1(ctx.state.snapshot.predicted_intercept_position.y)
            + ") | off_boresight=" + format_fixed_1(ctx.state.snapshot.off_boresight_deg)
            + " deg | Command visual " + (command_visual_active(ctx.state) ? "ON" : "OFF");
    } else {
        block += "Measurement=(" + format_fixed_1(ctx.state.snapshot.track.measurement_position.x)
            + "," + format_fixed_1(ctx.state.snapshot.track.measurement_position.y)
            + ") valid=" + (ctx.state.snapshot.track.measurement_valid ? "yes" : "no")
            + " | off_boresight=n/a | Command visual " + std::string(command_visual_active(ctx.state) ? "ON" : "OFF");
    }
    draw_text(renderer, ctx.body_font, panel.x + 12, panel.y + 42, rgba(220, 223, 230), block, panel.w - 24);
}

void render_event_panel(SDL_Renderer* renderer, const RenderContext& ctx) {
    const auto& panel = ctx.layout.event_panel;
    fill_panel(renderer, panel, rgba(8, 10, 14), rgba(54, 60, 78));
    const bool showing_review = ctx.state.review.visible && ctx.state.review.loaded;
    draw_text(renderer, ctx.title_font, panel.x + 12, panel.y + 10, rgba(164, 215, 150), "Event Timeline");
    draw_text(renderer,
              ctx.body_font,
              panel.x + 190,
              panel.y + 14,
              rgba(120, 128, 144),
              showing_review ? "[review mode]" : "[live mode]");
    const std::string events_block = terminal_timeline_text(ctx.state, showing_review);
    draw_text(renderer, ctx.body_font, panel.x + 12, panel.y + 46, rgba(187, 255, 187), events_block, panel.w - 24);
}

}  // namespace icss::viewer_gui
