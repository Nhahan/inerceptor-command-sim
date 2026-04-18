#include "app.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <fstream>
#include <stdexcept>
#include <string>

#include "icss/view/ascii_tactical_view.hpp"

namespace icss::viewer_gui {

SDL_Color rgba(Uint8 r, Uint8 g, Uint8 b, Uint8 a) {
    return SDL_Color {r, g, b, a};
}

void fill_panel(SDL_Renderer* renderer, const SDL_Rect& rect, SDL_Color fill, SDL_Color border) {
    SDL_SetRenderDrawColor(renderer, fill.r, fill.g, fill.b, fill.a);
    SDL_RenderFillRect(renderer, &rect);
    SDL_SetRenderDrawColor(renderer, border.r, border.g, border.b, border.a);
    SDL_RenderDrawRect(renderer, &rect);
}

void draw_text(SDL_Renderer* renderer,
               TTF_Font* font,
               int x,
               int y,
               SDL_Color color,
               const std::string& text,
               int wrap_width) {
    SDL_Surface* surface = wrap_width > 0
        ? TTF_RenderUTF8_Blended_Wrapped(font, text.c_str(), color, wrap_width)
        : TTF_RenderUTF8_Blended(font, text.c_str(), color);
    if (!surface) {
        throw std::runtime_error("failed to render text surface");
    }
    SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
    if (!texture) {
        SDL_FreeSurface(surface);
        throw std::runtime_error("failed to create text texture");
    }
    SDL_Rect dest {x, y, surface->w, surface->h};
    SDL_RenderCopy(renderer, texture, nullptr, &dest);
    SDL_DestroyTexture(texture);
    SDL_FreeSurface(surface);
}

namespace {

float map_axis(int value, int world_limit, int screen_origin, int screen_extent) {
    if (world_limit <= 1) {
        return static_cast<float>(screen_origin);
    }
    const int clamped = std::clamp(value, 0, world_limit - 1);
    return static_cast<float>(screen_origin) + (static_cast<float>(clamped) * static_cast<float>(screen_extent - 1) / static_cast<float>(world_limit - 1));
}

SDL_FPoint map_point_for_entity(const SDL_Rect& map_rect,
                                int world_width,
                                int world_height,
                                const icss::core::Vec2f& position) {
    return SDL_FPoint {
        map_axis(static_cast<int>(std::lround(position.x)), world_width, map_rect.x, map_rect.w),
        map_axis(static_cast<int>(std::lround(position.y)), world_height, map_rect.y, map_rect.h),
    };
}

bool rects_overlap(const SDL_Rect& lhs, const SDL_Rect& rhs) {
    return lhs.x < rhs.x + rhs.w
        && lhs.x + lhs.w > rhs.x
        && lhs.y < rhs.y + rhs.h
        && lhs.y + lhs.h > rhs.y;
}

std::vector<Button> make_control_buttons(const SDL_Rect& control_panel) {
    const int row1_y = control_panel.y + 84;
    const int row2_y = control_panel.y + 126;
    const int row3_y = control_panel.y + 176;
    return {
        {"Start", {control_panel.x + 12, row1_y, 82, 32}},
        {"Track", {control_panel.x + 102, row1_y, 82, 32}},
        {"Activate", {control_panel.x + 192, row1_y, 96, 32}},
        {"Command", {control_panel.x + 12, row2_y, 96, 32}},
        {"Stop", {control_panel.x + 116, row2_y, 72, 32}},
        {"Reset", {control_panel.x + 196, row2_y, 82, 32}},
        {"Review", {control_panel.x + 286, row2_y, 72, 32}},
        {"Balanced", {control_panel.x + 12, row3_y, 102, 24}},
        {"Evasive", {control_panel.x + 122, row3_y, 102, 24}},
        {"Timeout", {control_panel.x + 232, row3_y, 102, 24}},
    };
}

}  // namespace

GuiLayout build_layout(const ViewerOptions& options) {
    constexpr int margin = 24;
    constexpr int header_x = 24;
    constexpr int header_y = 18;
    constexpr int header_h = 64;
    constexpr int gap_after_header = 36;
    constexpr int gap_after_map = 18;
    constexpr int gap_before_event_panel = 18;
    constexpr int grid_width = 24;
    constexpr int grid_height = 16;
    constexpr int cell_px = 24;
    constexpr int entity_panel_h = 98;
    constexpr int right_panel_gap = 12;
    constexpr int left_panel_w = 330;

    GuiLayout layout;
    layout.header_panel = {header_x, header_y, options.width - (margin * 2), header_h};
    layout.phase_strip = {layout.header_panel.x + 12, layout.header_panel.y + 12, 330, layout.header_panel.h - 24};
    const int content_y = header_y + header_h + gap_after_header;
    layout.map_rect = {margin, content_y, grid_width * cell_px, grid_height * cell_px};
    layout.entity_panel = {margin, layout.map_rect.y + layout.map_rect.h + gap_after_map, layout.map_rect.w, entity_panel_h};
    layout.panel_x = layout.map_rect.x + layout.map_rect.w + margin;
    const int stacked_column_total_h = layout.map_rect.h + layout.entity_panel.h;
    const int panel_h = stacked_column_total_h / 2;
    layout.phase_panel = {layout.panel_x, content_y, left_panel_w, panel_h};
    const int right_panel_x = layout.panel_x + left_panel_w + right_panel_gap;
    layout.decision_panel = {right_panel_x, content_y, options.width - right_panel_x - margin, panel_h};
    layout.resilience_panel = {layout.panel_x, content_y + panel_h + right_panel_gap, left_panel_w, panel_h};
    layout.control_panel = {right_panel_x, content_y + panel_h + right_panel_gap, options.width - right_panel_x - margin, panel_h};
    const int upper_panel_bottom = std::max(layout.entity_panel.y + layout.entity_panel.h,
                                            std::max(layout.resilience_panel.y + layout.resilience_panel.h,
                                                     layout.control_panel.y + layout.control_panel.h));
    layout.event_panel = {margin, upper_panel_bottom + gap_before_event_panel, options.width - (margin * 2),
                          options.height - (upper_panel_bottom + gap_before_event_panel) - margin};
    layout.buttons = make_control_buttons(layout.control_panel);

    const std::array<SDL_Rect, 7> panels {
        layout.header_panel,
        layout.map_rect,
        layout.entity_panel,
        layout.phase_panel,
        layout.decision_panel,
        layout.resilience_panel,
        layout.control_panel,
    };
    for (std::size_t i = 0; i < panels.size(); ++i) {
        for (std::size_t j = i + 1; j < panels.size(); ++j) {
            if (rects_overlap(panels[i], panels[j])) {
                throw std::runtime_error("gui layout overlap detected");
            }
        }
    }
    if (rects_overlap(layout.entity_panel, layout.event_panel)
        || rects_overlap(layout.phase_panel, layout.event_panel)
        || rects_overlap(layout.decision_panel, layout.event_panel)
        || rects_overlap(layout.resilience_panel, layout.event_panel)
        || rects_overlap(layout.control_panel, layout.event_panel)) {
        throw std::runtime_error("gui event panel overlaps another panel");
    }
    for (std::size_t i = 0; i < layout.buttons.size(); ++i) {
        const auto& rect = layout.buttons[i].rect;
        const auto inside_control = rect.x >= layout.control_panel.x
            && rect.y >= layout.control_panel.y
            && rect.x + rect.w <= layout.control_panel.x + layout.control_panel.w
            && rect.y + rect.h <= layout.control_panel.y + layout.control_panel.h;
        if (!inside_control) {
            throw std::runtime_error("gui control button exceeds control panel bounds");
        }
        for (std::size_t j = i + 1; j < layout.buttons.size(); ++j) {
            if (rects_overlap(rect, layout.buttons[j].rect)) {
                throw std::runtime_error("gui control buttons overlap");
            }
        }
    }
    return layout;
}

void render_gui(SDL_Renderer* renderer,
                TTF_Font* title_font,
                TTF_Font* body_font,
                const ViewerState& state,
                const ViewerOptions& options) {
    constexpr int reference_step = 48;
    const auto layout = build_layout(options);
    const auto& header_panel = layout.header_panel;
    const auto& phase_strip = layout.phase_strip;
    const auto& map_rect = layout.map_rect;
    const auto& entity_panel = layout.entity_panel;
    const auto& phase_panel = layout.phase_panel;
    const auto& decision_panel = layout.decision_panel;
    const auto& resilience_panel = layout.resilience_panel;
    const auto& control_panel = layout.control_panel;
    const auto& event_panel = layout.event_panel;
    const auto& buttons = layout.buttons;
    const auto phase_title = phase_banner_label(state.snapshot.phase);
    const auto phase_note = phase_operator_note(state.snapshot.phase);
    const auto phase_color = phase_accent(state.snapshot.phase);
    const auto authoritative_title = authoritative_headline(state);
    const auto authoritative_badge = authoritative_badge_label(state);
    const auto authoritative_color = authoritative_badge_color(state);
    const auto resilience_note = resilience_summary(state);
    const auto recommended_control = recommended_control_label(state);
    const int title_font_h = TTF_FontHeight(title_font);
    const int body_font_h = TTF_FontHeight(body_font);
    const int header_text_gap = 4;
    const int header_text_total_h = title_font_h + header_text_gap + body_font_h;
    const int header_text_y = header_panel.y + std::max(0, (header_panel.h - header_text_total_h) / 2);

    SDL_SetRenderDrawColor(renderer, 15, 18, 24, 255);
    SDL_RenderClear(renderer);

    fill_panel(renderer, header_panel, rgba(18, 24, 32), rgba(56, 66, 86));
    SDL_SetRenderDrawColor(renderer, phase_color.r, phase_color.g, phase_color.b, 255);
    SDL_RenderFillRect(renderer, &phase_strip);
    draw_text(renderer, title_font, phase_strip.x + 14, phase_strip.y + 8, rgba(10, 12, 16), phase_title);
    const SDL_Rect status_badge {header_panel.x + header_panel.w - 196, header_panel.y + (header_panel.h - 28) / 2, 184, 28};
    fill_panel(renderer, status_badge, rgba(28, 34, 46), authoritative_color);
    draw_text(renderer,
              body_font,
              status_badge.x + 10,
              status_badge.y + std::max(0, (status_badge.h - body_font_h) / 2),
              authoritative_color,
              authoritative_badge,
              status_badge.w - 20);
    draw_text(renderer, title_font, header_panel.x + 360, header_text_y, rgba(236, 239, 244), "ICSS Tactical Viewer");
    draw_text(renderer,
              body_font,
              header_panel.x + 360,
              header_text_y + title_font_h + header_text_gap,
              rgba(200, 208, 220),
              phase_note,
              status_badge.x - (header_panel.x + 372));

    fill_panel(renderer, map_rect, rgba(36, 41, 52), rgba(74, 80, 96));
    SDL_SetRenderDrawColor(renderer, 74, 80, 96, 255);
    for (int x = 0; x <= map_rect.w; x += reference_step) {
        const int px = map_rect.x + x;
        SDL_RenderDrawLine(renderer, px, map_rect.y, px, map_rect.y + map_rect.h);
    }
    for (int y = 0; y <= map_rect.h; y += reference_step) {
        const int py = map_rect.y + y;
        SDL_RenderDrawLine(renderer, map_rect.x, py, map_rect.x + map_rect.w, py);
    }
    for (int x = 0; x <= state.snapshot.world_width; x += 96) {
        const int px = static_cast<int>(map_axis(x, state.snapshot.world_width, map_rect.x, map_rect.w));
        draw_text(renderer, body_font, px + 2, map_rect.y - 22, rgba(140, 149, 168), std::to_string(x));
    }
    for (int y = 0; y <= state.snapshot.world_height; y += 96) {
        const int py = static_cast<int>(map_axis(y, state.snapshot.world_height, map_rect.y, map_rect.h));
        draw_text(renderer, body_font, map_rect.x - 24, py + 2, rgba(140, 149, 168), std::to_string(y));
    }

    auto draw_history = [&](const std::deque<icss::core::Vec2>& history, SDL_Color color) {
        if (history.size() < 2) {
            return;
        }
        std::vector<SDL_Point> points;
        points.reserve(history.size());
        for (const auto& pos : history) {
            points.push_back(SDL_Point {
                static_cast<int>(map_axis(pos.x, state.snapshot.world_width, map_rect.x, map_rect.w)),
                static_cast<int>(map_axis(pos.y, state.snapshot.world_height, map_rect.y, map_rect.h)),
            });
        }
        SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, 150);
        SDL_RenderDrawLines(renderer, points.data(), static_cast<int>(points.size()));
    };
    draw_history(state.target_history, rgba(244, 67, 54, 160));
    draw_history(state.asset_history, rgba(66, 165, 245, 160));
    if (state.snapshot.track.active && state.snapshot.target.active && state.snapshot.asset.active) {
        const auto target_center = map_point_for_entity(map_rect, state.snapshot.world_width, state.snapshot.world_height, state.snapshot.target_world_position);
        const auto asset_center = map_point_for_entity(map_rect, state.snapshot.world_width, state.snapshot.world_height, state.snapshot.asset_world_position);
        if (state.snapshot.predicted_intercept_valid) {
            const auto predicted_center = map_point_for_entity(
                map_rect,
                state.snapshot.world_width,
                state.snapshot.world_height,
                state.snapshot.predicted_intercept_position);
            SDL_SetRenderDrawColor(renderer, 182, 255, 182, 220);
            SDL_RenderDrawLine(renderer,
                               static_cast<int>(predicted_center.x) - 8,
                               static_cast<int>(predicted_center.y),
                               static_cast<int>(predicted_center.x) + 8,
                               static_cast<int>(predicted_center.y));
            SDL_RenderDrawLine(renderer,
                               static_cast<int>(predicted_center.x),
                               static_cast<int>(predicted_center.y) - 8,
                               static_cast<int>(predicted_center.x),
                               static_cast<int>(predicted_center.y) + 8);
        }
        SDL_SetRenderDrawColor(renderer, 255, 214, 102, 180);
        SDL_RenderDrawLine(renderer,
                           static_cast<int>(asset_center.x),
                           static_cast<int>(asset_center.y),
                           static_cast<int>(target_center.x),
                           static_cast<int>(target_center.y));
        if (state.command_visual_active || state.snapshot.command_status != icss::core::CommandLifecycle::None) {
            SDL_SetRenderDrawColor(renderer, 255, 149, 0, 255);
            SDL_RenderDrawLine(renderer,
                               static_cast<int>(asset_center.x) - 1,
                               static_cast<int>(asset_center.y),
                               static_cast<int>(target_center.x) - 1,
                               static_cast<int>(target_center.y));
            SDL_RenderDrawLine(renderer,
                               static_cast<int>(asset_center.x) + 1,
                               static_cast<int>(asset_center.y),
                               static_cast<int>(target_center.x) + 1,
                               static_cast<int>(target_center.y));
            SDL_Rect target_box {
                static_cast<int>(target_center.x) - 10,
                static_cast<int>(target_center.y) - 10,
                20,
                20,
            };
            SDL_RenderDrawRect(renderer, &target_box);
        }
        SDL_SetRenderDrawColor(renderer, 244, 67, 54, 180);
        SDL_RenderDrawLine(renderer,
                           static_cast<int>(target_center.x),
                           static_cast<int>(target_center.y),
                           static_cast<int>(target_center.x + state.snapshot.target_velocity.x * 6.0F),
                           static_cast<int>(target_center.y + state.snapshot.target_velocity.y * 6.0F));
        SDL_SetRenderDrawColor(renderer, 66, 165, 245, 180);
        SDL_RenderDrawLine(renderer,
                           static_cast<int>(asset_center.x),
                           static_cast<int>(asset_center.y),
                           static_cast<int>(asset_center.x + state.snapshot.asset_velocity.x * 6.0F),
                           static_cast<int>(asset_center.y + state.snapshot.asset_velocity.y * 6.0F));
    }

    auto draw_entity = [&](const icss::core::EntityState& entity, SDL_Color color) {
        SDL_Rect rect {0, 0, 8, 8};
        const auto center = (&entity == &state.snapshot.target)
            ? map_point_for_entity(map_rect, state.snapshot.world_width, state.snapshot.world_height, state.snapshot.target_world_position)
            : map_point_for_entity(map_rect, state.snapshot.world_width, state.snapshot.world_height, state.snapshot.asset_world_position);
        rect.x = static_cast<int>(center.x) - 4;
        rect.y = static_cast<int>(center.y) - 4;
        if (entity.active) {
            SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);
            SDL_RenderFillRect(renderer, &rect);
            SDL_SetRenderDrawColor(renderer, 240, 244, 255, 255);
            SDL_RenderDrawRect(renderer, &rect);
        } else {
            SDL_SetRenderDrawColor(renderer, color.r / 3, color.g / 3, color.b / 3, 255);
            SDL_RenderDrawRect(renderer, &rect);
        }
    };
    draw_entity(state.snapshot.target, rgba(244, 67, 54));
    draw_entity(state.snapshot.asset, rgba(66, 165, 245));

    fill_panel(renderer, phase_panel, rgba(12, 14, 20), rgba(54, 60, 78));
    draw_text(renderer, title_font, phase_panel.x + 12, phase_panel.y + 10, rgba(141, 211, 199), "Mission Phase");
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
        const SDL_Rect row {phase_panel.x + 12, phase_panel.y + 42 + static_cast<int>(index) * 20, phase_panel.w - 24, 18};
        const auto row_phase = phase_flow[index].first;
        const auto active = row_phase == state.snapshot.phase;
        const auto completed = static_cast<int>(row_phase) < static_cast<int>(state.snapshot.phase);
        if (active) {
            SDL_SetRenderDrawColor(renderer, phase_color.r, phase_color.g, phase_color.b, 255);
            SDL_RenderFillRect(renderer, &row);
            draw_text(renderer, body_font, row.x + 8, row.y + 1, rgba(10, 12, 16), phase_flow[index].second);
        } else {
            SDL_SetRenderDrawColor(renderer, completed ? 68 : 42, completed ? 91 : 50, completed ? 72 : 58, 255);
            SDL_RenderFillRect(renderer, &row);
            SDL_SetRenderDrawColor(renderer, 66, 74, 92, 255);
            SDL_RenderDrawRect(renderer, &row);
            draw_text(renderer,
                      body_font,
                      row.x + 8,
                      row.y + 1,
                      completed ? rgba(176, 216, 184) : rgba(148, 156, 172),
                      phase_flow[index].second);
        }
    }

    fill_panel(renderer, decision_panel, rgba(12, 14, 20), rgba(54, 60, 78));
    draw_text(renderer, title_font, decision_panel.x + 12, decision_panel.y + 10, rgba(255, 179, 102), "Authoritative Status");
    std::string decision_block;
    decision_block += "Last telemetry event: " + telemetry_event_status(state) + "\n";
    decision_block += "Latest timeline entry: " + latest_timeline_entry(state) + "\n";
    decision_block += std::string(state.control.last_ok ? "Last accepted control: " : "Last rejected control: ")
        + state.control.last_label + " | " + state.control.last_message + "\n";
    decision_block += "Judgment: " + std::string(icss::core::to_string(state.snapshot.judgment.code))
        + " | Command lifecycle: " + std::string(icss::core::to_string(state.snapshot.command_status)) + "\n";
    decision_block += "Interceptor status: " + std::string(icss::core::to_string(state.snapshot.asset_status))
        + " | AAR cursor: live";
    draw_text(renderer,
              body_font,
              decision_panel.x + 12,
              decision_panel.y + 44,
              rgba(220, 223, 230),
              decision_block,
              decision_panel.w - 24);

    fill_panel(renderer, resilience_panel, rgba(12, 14, 20), rgba(54, 60, 78));
    draw_text(renderer, title_font, resilience_panel.x + 12, resilience_panel.y + 10, rgba(129, 199, 255), "Resilience / Telemetry");
    std::string resilience_block;
    resilience_block += "Connection: " + std::string(icss::core::to_string(state.snapshot.viewer_connection))
        + " | Freshness: " + icss::view::freshness_label(state.snapshot) + "\n";
    resilience_block += "Tick: " + std::to_string(state.snapshot.telemetry.tick)
        + " | Snapshot: " + std::to_string(state.snapshot.header.snapshot_sequence) + "\n";
    resilience_block += "Latency: " + format_fixed_0(state.snapshot.telemetry.latency_ms)
        + " ms | Loss: " + format_fixed_1(state.snapshot.telemetry.packet_loss_pct) + " %";
    if (state.snapshot.telemetry.packet_loss_pct > 0.0F) {
        resilience_block += " | degraded sample";
    } else {
        resilience_block += " | stable";
    }
    resilience_block += "\n";
    resilience_block += "I/O: snapshots=" + std::to_string(state.snapshot_count_received)
        + ", telemetry=" + std::to_string(state.telemetry_count_received)
        + ", heartbeats=" + std::to_string(state.heartbeat_count) + "\n";
    resilience_block += "Note: " + resilience_note;
    draw_text(renderer,
              body_font,
              resilience_panel.x + 12,
              resilience_panel.y + 44,
              rgba(220, 223, 230),
              resilience_block,
              resilience_panel.w - 24);

    fill_panel(renderer, control_panel, rgba(12, 14, 20), rgba(54, 60, 78));
    draw_text(renderer, title_font, control_panel.x + 12, control_panel.y + 10, rgba(141, 211, 199), "Control Panel");
    const SDL_Rect next_hint_box {control_panel.x + 12, control_panel.y + 40, control_panel.w - 24, 30};
    fill_panel(renderer, next_hint_box, rgba(22, 27, 36), rgba(56, 66, 86));
    const SDL_Rect profile_box {control_panel.x + 12, control_panel.y + 204, control_panel.w - 24, 28};
    fill_panel(renderer, profile_box, rgba(22, 27, 36), rgba(56, 66, 86));
    for (const auto& button : buttons) {
        const bool is_review_button = button.label == "Review";
        const bool is_reset_button = button.label == "Reset";
        const bool is_profile = is_profile_button(button.label);
        const bool enabled = !is_review_button || review_available(state);
        const bool recommended = enabled && button.label == recommended_control;
        const auto fill = is_reset_button
            ? (enabled ? rgba(92, 42, 52) : rgba(56, 36, 40))
            : (is_profile
                ? (state.profile_label == button.label ? rgba(76, 92, 126) : rgba(34, 44, 58))
                : (recommended ? phase_color : (enabled ? rgba(28, 55, 86) : rgba(38, 38, 38))));
        const auto border = is_reset_button
            ? (enabled ? rgba(224, 108, 146) : rgba(96, 72, 80))
            : (is_profile
                ? (state.profile_label == button.label ? rgba(174, 195, 255) : rgba(88, 102, 126))
                : (recommended ? rgba(240, 244, 255) : (enabled ? rgba(91, 126, 168) : rgba(72, 72, 72))));
        const auto text = is_reset_button
            ? (enabled ? rgba(255, 220, 230) : rgba(148, 156, 172))
            : (is_profile
                ? (state.profile_label == button.label ? rgba(238, 245, 255) : rgba(188, 198, 214))
                : (recommended ? rgba(10, 12, 16) : (enabled ? rgba(238, 245, 255) : rgba(148, 156, 172))));
        if (is_reset_button && enabled) {
            const SDL_Rect glow_outer {button.rect.x - 1, button.rect.y - 1, button.rect.w + 2, button.rect.h + 2};
            SDL_SetRenderDrawColor(renderer, 255, 128, 168, 96);
            SDL_RenderDrawRect(renderer, &glow_outer);
        }
        SDL_SetRenderDrawColor(renderer, fill.r, fill.g, fill.b, fill.a);
        SDL_RenderFillRect(renderer, &button.rect);
        SDL_SetRenderDrawColor(renderer, border.r, border.g, border.b, border.a);
        SDL_RenderDrawRect(renderer, &button.rect);
        if (is_reset_button) {
            const SDL_Rect inner {button.rect.x + 1, button.rect.y + 1, button.rect.w - 2, button.rect.h - 2};
            SDL_SetRenderDrawColor(renderer, 255, 168, 196, enabled ? 168 : 84);
            SDL_RenderDrawRect(renderer, &inner);
        }
        if (recommended) {
            const SDL_Rect glow {button.rect.x - 2, button.rect.y - 2, button.rect.w + 4, button.rect.h + 4};
            SDL_SetRenderDrawColor(renderer, 255, 255, 255, 180);
            SDL_RenderDrawRect(renderer, &glow);
        }
        draw_text(renderer, body_font, button.rect.x + 10, button.rect.y + 7, text, button.label);
    }
    draw_text(renderer,
              body_font,
              next_hint_box.x + 10,
              next_hint_box.y + 7,
              rgba(148, 156, 172),
              recommended_control.empty()
                  ? "Next recommended control: none"
                  : "Next recommended control: " + recommended_control,
              next_hint_box.w - 20);
    draw_text(renderer,
              body_font,
              profile_box.x + 10,
              profile_box.y + 7,
              rgba(148, 156, 172),
              "Profile=" + state.profile_label
                  + " | vT=(" + std::to_string(state.planned_scenario.target_velocity_x) + "," + std::to_string(state.planned_scenario.target_velocity_y) + ")"
                  + " | vI=" + std::to_string(state.planned_scenario.interceptor_speed_per_tick)
                  + " | timeout=" + std::to_string(state.planned_scenario.engagement_timeout_ticks),
              profile_box.w - 20);

    fill_panel(renderer, entity_panel, rgba(12, 14, 20), rgba(54, 60, 78));
    draw_text(renderer, title_font, entity_panel.x + 12, entity_panel.y + 10, rgba(220, 223, 230), "Entity Picture");
    draw_text(renderer, body_font, entity_panel.x + 170, entity_panel.y + 14, rgba(140, 149, 168), "target=red | interceptor=blue | command=orange");
    std::string entity_block;
    entity_block += "Target " + state.snapshot.target.id + " @ (" + format_fixed_1(state.snapshot.target_world_position.x)
        + ", " + format_fixed_1(state.snapshot.target_world_position.y) + ") active=" + (state.snapshot.target.active ? "yes" : "no") + "\n";
    entity_block += "Interceptor " + state.snapshot.asset.id + " @ (" + format_fixed_1(state.snapshot.asset_world_position.x)
        + ", " + format_fixed_1(state.snapshot.asset_world_position.y) + ") active=" + (state.snapshot.asset.active ? "yes" : "no") + "\n";
    entity_block += "Target v=(" + format_fixed_1(state.snapshot.target_velocity.x) + "," + format_fixed_1(state.snapshot.target_velocity.y)
        + ") hdg=" + format_fixed_1(state.snapshot.target_heading_deg)
        + " | Interceptor v=(" + format_fixed_1(state.snapshot.asset_velocity.x) + "," + format_fixed_1(state.snapshot.asset_velocity.y)
        + ") hdg=" + format_fixed_1(state.snapshot.asset_heading_deg) + "\n";
    entity_block += "Tracking " + std::string(state.snapshot.track.active ? "ON" : "OFF")
        + " (" + std::to_string(state.snapshot.track.confidence_pct) + "%)"
        + " | TTI=" + format_fixed_1(state.snapshot.time_to_intercept_s)
        + "s | seeker_lock=" + (state.snapshot.seeker_lock ? "yes" : "no") + "\n";
    entity_block += "Predicted intercept=(" + format_fixed_1(state.snapshot.predicted_intercept_position.x)
        + "," + format_fixed_1(state.snapshot.predicted_intercept_position.y)
        + ") | off_boresight=" + format_fixed_1(state.snapshot.off_boresight_deg)
        + " deg | Command visual " + (state.command_visual_active ? "ON" : "OFF");
    draw_text(renderer, body_font, entity_panel.x + 12, entity_panel.y + 42, rgba(220, 223, 230), entity_block, entity_panel.w - 24);

    fill_panel(renderer, event_panel, rgba(8, 10, 14), rgba(54, 60, 78));
    const bool showing_review = state.review.visible && state.review.loaded;
    draw_text(renderer,
              title_font,
              event_panel.x + 12,
              event_panel.y + 10,
              rgba(164, 215, 150),
              "Event Timeline");
    draw_text(renderer,
              body_font,
              event_panel.x + 190,
              event_panel.y + 14,
              rgba(120, 128, 144),
              showing_review ? "[review mode]" : "[live mode]");

    const std::string events_block = terminal_timeline_text(state, showing_review);
    draw_text(renderer,
              body_font,
              event_panel.x + 12,
              event_panel.y + 46,
              rgba(187, 255, 187),
              events_block,
              event_panel.w - 24);

    SDL_RenderPresent(renderer);
}

void write_dump_state(const std::filesystem::path& path, const ViewerState& state) {
    if (path.empty()) {
        return;
    }
    std::filesystem::create_directories(path.parent_path());
    std::ofstream out(path);
    out << "{\n";
    out << "  \"schema_version\": \"icss-gui-viewer-state-v1\",\n";
    out << "  \"received_snapshot\": " << (state.received_snapshot ? "true" : "false") << ",\n";
    out << "  \"received_telemetry\": " << (state.received_telemetry ? "true" : "false") << ",\n";
    out << "  \"snapshot_sequence\": " << state.snapshot.header.snapshot_sequence << ",\n";
    out << "  \"tick\": " << state.snapshot.telemetry.tick << ",\n";
    out << "  \"phase\": \"" << icss::core::to_string(state.snapshot.phase) << "\",\n";
    out << "  \"phase_banner\": \"" << escape_json(phase_banner_label(state.snapshot.phase)) << "\",\n";
    out << "  \"profile_label\": \"" << escape_json(state.profile_label) << "\",\n";
    out << "  \"connection_state\": \"" << icss::core::to_string(state.snapshot.viewer_connection) << "\",\n";
    out << "  \"freshness\": \"" << icss::view::freshness_label(state.snapshot) << "\",\n";
    out << "  \"resilience_summary\": \"" << escape_json(resilience_summary(state)) << "\",\n";
    out << "  \"judgment_code\": \"" << icss::core::to_string(state.snapshot.judgment.code) << "\",\n";
    out << "  \"heartbeat_count\": " << state.heartbeat_count << ",\n";
    out << "  \"last_server_event_tick\": " << state.last_server_event_tick << ",\n";
    out << "  \"last_server_event_type\": \"" << escape_json(state.last_server_event_type) << "\",\n";
    out << "  \"last_server_event_summary\": \"" << escape_json(state.last_server_event_summary) << "\",\n";
    out << "  \"authoritative_headline\": \"" << escape_json(authoritative_headline(state)) << "\",\n";
    out << "  \"recommended_control\": \"" << escape_json(recommended_control_label(state)) << "\",\n";
    out << "  \"review_available\": " << (review_available(state) ? "true" : "false") << ",\n";
    out << "  \"review_loaded\": " << (state.review.loaded ? "true" : "false") << ",\n";
    out << "  \"review_visible\": " << (state.review.visible ? "true" : "false") << ",\n";
    out << "  \"review_cursor_index\": " << state.review.cursor_index << ",\n";
    out << "  \"review_total_events\": " << state.review.total_events << ",\n";
    out << "  \"review_judgment_code\": \"" << escape_json(state.review.judgment_code) << "\",\n";
    out << "  \"review_resilience_case\": \"" << escape_json(state.review.resilience_case) << "\",\n";
    out << "  \"review_event_type\": \"" << escape_json(state.review.event_type) << "\",\n";
    out << "  \"review_event_summary\": \"" << escape_json(state.review.event_summary) << "\",\n";
    out << "  \"last_control_label\": \"" << escape_json(state.control.last_label) << "\",\n";
    out << "  \"last_control_message\": \"" << escape_json(state.control.last_message) << "\",\n";
    out << "  \"asset_status\": \"" << icss::core::to_string(state.snapshot.asset_status) << "\",\n";
    out << "  \"command_status\": \"" << icss::core::to_string(state.snapshot.command_status) << "\",\n";
    out << "  \"command_visual_active\": " << (state.command_visual_active ? "true" : "false") << ",\n";
    out << "  \"target_active\": " << (state.snapshot.target.active ? "true" : "false") << ",\n";
    out << "  \"asset_active\": " << (state.snapshot.asset.active ? "true" : "false") << ",\n";
    out << "  \"target_x\": " << state.snapshot.target.position.x << ",\n";
    out << "  \"target_y\": " << state.snapshot.target.position.y << ",\n";
    out << "  \"target_velocity_x\": " << state.snapshot.target_velocity_x << ",\n";
    out << "  \"target_velocity_y\": " << state.snapshot.target_velocity_y << ",\n";
    out << "  \"target_world_x\": " << state.snapshot.target_world_position.x << ",\n";
    out << "  \"target_world_y\": " << state.snapshot.target_world_position.y << ",\n";
    out << "  \"target_heading_deg\": " << state.snapshot.target_heading_deg << ",\n";
    out << "  \"asset_x\": " << state.snapshot.asset.position.x << ",\n";
    out << "  \"asset_y\": " << state.snapshot.asset.position.y << ",\n";
    out << "  \"asset_world_x\": " << state.snapshot.asset_world_position.x << ",\n";
    out << "  \"asset_world_y\": " << state.snapshot.asset_world_position.y << ",\n";
    out << "  \"asset_heading_deg\": " << state.snapshot.asset_heading_deg << ",\n";
    out << "  \"world_width\": " << state.snapshot.world_width << ",\n";
    out << "  \"world_height\": " << state.snapshot.world_height << ",\n";
    out << "  \"interceptor_speed_per_tick\": " << state.snapshot.interceptor_speed_per_tick << ",\n";
    out << "  \"interceptor_acceleration_per_tick\": " << state.snapshot.interceptor_acceleration_per_tick << ",\n";
    out << "  \"engagement_timeout_ticks\": " << state.snapshot.engagement_timeout_ticks << ",\n";
    out << "  \"seeker_fov_deg\": " << state.snapshot.seeker_fov_deg << ",\n";
    out << "  \"seeker_lock\": " << (state.snapshot.seeker_lock ? "true" : "false") << ",\n";
    out << "  \"off_boresight_deg\": " << state.snapshot.off_boresight_deg << ",\n";
    out << "  \"predicted_intercept_valid\": " << (state.snapshot.predicted_intercept_valid ? "true" : "false") << ",\n";
    out << "  \"predicted_intercept_x\": " << state.snapshot.predicted_intercept_position.x << ",\n";
    out << "  \"predicted_intercept_y\": " << state.snapshot.predicted_intercept_position.y << ",\n";
    out << "  \"time_to_intercept_s\": " << state.snapshot.time_to_intercept_s << ",\n";
    out << "  \"recent_event_count\": " << state.recent_server_events.size() << "\n";
    out << "}\n";
}

}  // namespace icss::viewer_gui
