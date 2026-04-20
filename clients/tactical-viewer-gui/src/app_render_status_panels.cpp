#include "render_internal.hpp"

#include <array>
#include <cmath>
#include <string_view>

#include "icss/view/ascii_tactical_view.hpp"

namespace icss::viewer_gui {
namespace {

std::string upper_label(std::string_view text) {
    return uppercase_words(text);
}

std::string connection_value(const RenderContext& ctx) {
    switch (ctx.state.snapshot.display_connection) {
    case icss::core::ConnectionState::Connected:
        return "ON NET";
    case icss::core::ConnectionState::Reconnected:
        return "LINK RESTORED";
    case icss::core::ConnectionState::TimedOut:
        return "TIMED OUT";
    case icss::core::ConnectionState::Disconnected:
        return "OFF NET";
    }
    return "UNKNOWN";
}

std::string picture_status_value(std::string_view freshness) {
    if (freshness == "current") {
        return "CURRENT";
    }
    if (freshness == "degraded") {
        return "DEGRADED";
    }
    if (freshness == "reacquiring") {
        return "REACQUIRING";
    }
    if (freshness == "stale") {
        return "STALE";
    }
    return "UNKNOWN";
}

std::string compact_phase_value(icss::core::SessionPhase phase) {
    switch (phase) {
    case icss::core::SessionPhase::Standby:
        return "STBY";
    case icss::core::SessionPhase::Detecting:
        return "DETECT";
    case icss::core::SessionPhase::Tracking:
        return "TRACK";
    case icss::core::SessionPhase::InterceptorReady:
        return "READY";
    case icss::core::SessionPhase::EngageOrdered:
        return "ORDERED";
    case icss::core::SessionPhase::Intercepting:
        return "FLIGHT";
    case icss::core::SessionPhase::Assessed:
        return "ASSESSED";
    case icss::core::SessionPhase::Archived:
        return "ARCHIVE";
    }
    return "UNKNOWN";
}

std::string compact_assessment_value(icss::core::AssessmentCode code) {
    switch (code) {
    case icss::core::AssessmentCode::Pending:
        return "PEND";
    case icss::core::AssessmentCode::InterceptSuccess:
        return "SUCCESS";
    case icss::core::AssessmentCode::TimeoutObserved:
        return "TIMEOUT";
    case icss::core::AssessmentCode::InvalidTransition:
        return "INVALID";
    }
    return "UNKNOWN";
}

SDL_Color freshness_color(std::string_view freshness) {
    if (freshness == "current") {
        return rgba(120, 208, 140);
    }
    if (freshness == "degraded") {
        return rgba(255, 189, 89);
    }
    if (freshness == "reacquiring") {
        return rgba(110, 190, 255);
    }
    return rgba(255, 110, 110);
}

std::uint64_t picture_age_ms(const RenderContext& ctx) {
    if (ctx.state.last_snapshot_wall_time_ms == 0 || ctx.state.now_wall_time_ms < ctx.state.last_snapshot_wall_time_ms) {
        return 0;
    }
    return ctx.state.now_wall_time_ms - ctx.state.last_snapshot_wall_time_ms;
}

SDL_Color link_delay_color(const RenderContext& ctx) {
    if (!ctx.state.has_link_delay_sample) {
        return rgba(148, 156, 172);
    }
    if (ctx.state.link_delay_ms >= 500U) {
        return rgba(255, 110, 110);
    }
    if (ctx.state.link_delay_ms >= 200U) {
        return rgba(255, 189, 89);
    }
    return rgba(120, 208, 140);
}

SDL_Color picture_age_color(const RenderContext& ctx, std::uint64_t age_ms) {
    const auto freshness = icss::view::freshness_label(ctx.state.snapshot);
    if (freshness == "stale") {
        return rgba(255, 110, 110);
    }
    if (freshness == "degraded" || age_ms > (ctx.state.snapshot.telemetry.tick > 0 ? 500U : 250U)) {
        return rgba(255, 189, 89);
    }
    if (freshness == "reacquiring") {
        return rgba(110, 190, 255);
    }
    return rgba(120, 208, 140);
}

std::string picture_age_value(const RenderContext& ctx, std::uint64_t age_ms) {
    if (ctx.state.last_snapshot_wall_time_ms == 0) {
        return "NO DATA";
    }
    return std::to_string(age_ms) + " ms";
}

std::string link_delay_value(const RenderContext& ctx) {
    if (!ctx.state.has_link_delay_sample) {
        return "NO DATA";
    }
    return std::to_string(ctx.state.link_delay_ms) + " ms";
}

void fill_section_panel(SDL_Renderer* renderer, const SDL_Rect& rect) {
    SDL_SetRenderDrawColor(renderer, 14, 18, 26, 228);
    SDL_RenderFillRect(renderer, &rect);
    SDL_SetRenderDrawColor(renderer, 56, 66, 86, 200);
    SDL_RenderDrawLine(renderer, rect.x, rect.y, rect.x + rect.w, rect.y);
}

void render_summary_row(SDL_Renderer* renderer,
                        const RenderContext& ctx,
                        const SDL_Rect& rect,
                        const std::string& left_value,
                        SDL_Color accent) {
    fill_panel(renderer, rect, rgba(18, 23, 32), rgba(56, 66, 86));
    SDL_Rect band {rect.x, rect.y, rect.w, 3};
    SDL_SetRenderDrawColor(renderer, accent.r, accent.g, accent.b, accent.a);
    SDL_RenderFillRect(renderer, &band);
    draw_text(renderer,
              ctx.compact_font,
              rect.x + 10,
              rect.y + 6,
              rgba(220, 223, 230),
              left_value,
              rect.w - 20);
}

SDL_Color timeline_line_color(std::string_view line, bool aar_mode) {
    if (line.empty()) {
        return rgba(148, 156, 172);
    }
    if (line.rfind("mode=", 0) == 0 || line.rfind("----------------------------------------", 0) == 0) {
        return rgba(126, 201, 126);
    }
    if (aar_mode) {
        if (line.find("Assessment:") != std::string_view::npos) {
            return rgba(255, 189, 89);
        }
        if (line.find("Resilience:") != std::string_view::npos) {
            return rgba(110, 190, 255);
        }
        if (line.find("Current event:") != std::string_view::npos) {
            return rgba(120, 208, 140);
        }
        return rgba(204, 255, 204);
    }
    if (line.find("[control rejected]") != std::string_view::npos || line.find("rejected") != std::string_view::npos) {
        return rgba(255, 136, 136);
    }
    if (line.find("[control accepted]") != std::string_view::npos) {
        return rgba(110, 190, 255);
    }
    if (line.find("Fire order executed") != std::string_view::npos || line.find("Engagement assessed") != std::string_view::npos) {
        return rgba(255, 206, 112);
    }
    if (line.find("[tick ") != std::string_view::npos) {
        return rgba(187, 255, 187);
    }
    return rgba(188, 198, 214);
}

}  // namespace

void render_phase_panel(SDL_Renderer* renderer, const RenderContext& ctx) {
    (void) renderer;
    (void) ctx;
}

void render_decision_panel(SDL_Renderer* renderer, const RenderContext& ctx) {
    const auto& panel = ctx.layout.decision_panel;
    const bool review_mode = ctx.layout.mode == LayoutMode::ReviewTactical;
    fill_section_panel(renderer, panel);
    draw_text(renderer,
              ctx.title_font,
              panel.x + 12,
              panel.y + 10,
              rgba(255, 179, 102),
              review_mode ? "Engagement Summary" : "Mission Summary");
    const SDL_Rect row1 {panel.x + 12, panel.y + 42, panel.w - 24, 26};
    const SDL_Rect row2 {panel.x + 12, row1.y + 32, panel.w - 24, 26};
    const SDL_Rect row3 {panel.x + 12, row2.y + 32, panel.w - 24, 26};
    render_summary_row(renderer,
                       ctx,
                       row1,
                       "State  " + compact_phase_value(ctx.state.snapshot.phase) + "  |  Asmt  "
                           + compact_assessment_value(ctx.state.snapshot.assessment.code),
                       ctx.phase_color);
    render_summary_row(renderer,
                       ctx,
                       row2,
                       "Wp  " + upper_label(icss::core::to_string(ctx.state.snapshot.interceptor_status))
                           + "  |  Ord  " + upper_label(icss::core::to_string(ctx.state.snapshot.engage_order_status)),
                       rgba(110, 190, 255));
    const std::string row3_value = review_mode
        ? ("Rev  "
            + (ctx.state.aar.loaded
                ? (std::to_string(ctx.state.aar.cursor_index) + "/" + std::to_string(ctx.state.aar.total_events))
                : std::string("AVAILABLE"))
            + "  |  Prof  " + std::string(ctx.state.effective_track_active ? "TRACK" : "UNGUIDED"))
        : ("Trk  " + std::string(ctx.state.snapshot.track.active ? "FILE" : "NONE")
            + "  |  Prof  " + std::string(ctx.state.effective_track_active ? "TRACK" : "UNGUIDED"));
    render_summary_row(renderer,
                       ctx,
                       row3,
                       row3_value,
                       review_mode ? rgba(164, 215, 150) : rgba(141, 211, 199));
}

void render_resilience_panel(SDL_Renderer* renderer, const RenderContext& ctx) {
    const auto& panel = ctx.layout.resilience_panel;
    fill_section_panel(renderer, panel);
    draw_text(renderer, ctx.title_font, panel.x + 12, panel.y + 10, rgba(129, 199, 255), "Link / Picture");
    const auto freshness = icss::view::freshness_label(ctx.state.snapshot);
    const auto age_ms = picture_age_ms(ctx);
    const SDL_Rect row1 {panel.x + 12, panel.y + 42, panel.w - 24, 26};
    const SDL_Rect row2 {panel.x + 12, row1.y + 32, panel.w - 24, 26};
    const SDL_Rect row3 {panel.x + 12, row2.y + 32, panel.w - 24, 42};
    render_summary_row(renderer,
                       ctx,
                       row1,
                       "Pic  " + picture_status_value(freshness) + "  |  Link  "
                           + (connection_value(ctx) == "ON NET" ? "ON-NET" : connection_value(ctx)),
                       freshness_color(freshness));
    render_summary_row(renderer,
                       ctx,
                       row2,
                       "Delay  " + link_delay_value(ctx) + "  |  Age  " + picture_age_value(ctx, age_ms),
                       ctx.state.has_link_delay_sample ? link_delay_color(ctx) : picture_age_color(ctx, age_ms));
    fill_panel(renderer, row3, rgba(18, 23, 32), rgba(56, 66, 86));
    SDL_Rect band {row3.x, row3.y, row3.w, 3};
    SDL_SetRenderDrawColor(renderer, 129, 199, 255, 255);
    SDL_RenderFillRect(renderer, &band);
    draw_text(renderer,
              ctx.compact_font,
              row3.x + 10,
              row3.y + 6,
              rgba(220, 223, 230),
              "Cycle  " + std::to_string(ctx.state.snapshot.telemetry.tick_interval_ms) + "  |  #"
                  + std::to_string(ctx.state.snapshot.header.snapshot_sequence),
              row3.w - 20);
}

void render_entity_panel(SDL_Renderer* renderer, const RenderContext& ctx) {
    (void) renderer;
    (void) ctx;
}

void render_event_panel(SDL_Renderer* renderer, const RenderContext& ctx) {
    const auto& panel = ctx.layout.event_panel;
    SDL_SetRenderDrawColor(renderer, 8, 10, 14, 220);
    SDL_RenderFillRect(renderer, &panel);
    SDL_SetRenderDrawColor(renderer, 54, 60, 78, 255);
    SDL_RenderDrawLine(renderer, panel.x, panel.y, panel.x + panel.w, panel.y);
    const bool showing_aar = ctx.state.aar.visible && ctx.state.aar.loaded;
    draw_text(renderer,
              ctx.title_font,
              panel.x + 12,
              panel.y + 10,
              rgba(164, 215, 150),
              showing_aar ? "Post-Engagement Review" : "Event Feed");
    auto lines = terminal_timeline_lines(ctx.state, showing_aar);
    const int line_height = std::max(16, TTF_FontLineSkip(ctx.body_font));
    const int scrollbar_w = 10;
    const SDL_Rect body_rect {panel.x + 12, panel.y + 46, panel.w - 36, panel.h - 58};
    const std::size_t visible_lines = std::max(1, body_rect.h / line_height);
    const std::size_t total_lines = lines.size();
    const std::size_t max_scroll = total_lines > visible_lines ? (total_lines - visible_lines) : 0;
    const std::size_t scroll = std::min(ctx.state.timeline_scroll_lines, max_scroll);
    const SDL_Rect text_clip {body_rect.x, body_rect.y, body_rect.w - scrollbar_w - 8, body_rect.h};
    SDL_RenderSetClipRect(renderer, &text_clip);
    const std::size_t end = std::min(total_lines, scroll + visible_lines);
    for (std::size_t index = scroll; index < end; ++index) {
        draw_text(renderer,
                  ctx.body_font,
                  body_rect.x,
                  body_rect.y + static_cast<int>((index - scroll) * line_height),
                  timeline_line_color(lines[index], showing_aar),
                  lines[index]);
    }
    SDL_RenderSetClipRect(renderer, nullptr);

    if (total_lines > visible_lines) {
        const SDL_Rect track {panel.x + panel.w - 18, body_rect.y, scrollbar_w, body_rect.h};
        SDL_SetRenderDrawColor(renderer, 28, 34, 46, 220);
        SDL_RenderFillRect(renderer, &track);
        SDL_SetRenderDrawColor(renderer, 54, 60, 78, 255);
        SDL_RenderDrawRect(renderer, &track);
        const int thumb_h = std::max(24, static_cast<int>((static_cast<float>(visible_lines) / static_cast<float>(total_lines)) * static_cast<float>(track.h)));
        const float denom = max_scroll == 0 ? 1.0F : static_cast<float>(max_scroll);
        const float thumb_ratio = static_cast<float>(scroll) / denom;
        const int thumb_y = track.y + static_cast<int>(thumb_ratio * static_cast<float>(track.h - thumb_h));
        SDL_Rect thumb {track.x + 1, thumb_y, track.w - 2, thumb_h};
        SDL_SetRenderDrawColor(renderer, 112, 192, 112, 230);
        SDL_RenderFillRect(renderer, &thumb);
        SDL_SetRenderDrawColor(renderer, 164, 215, 150, 255);
        SDL_RenderDrawRect(renderer, &thumb);
    }
}

}  // namespace icss::viewer_gui
