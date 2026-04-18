#include "render_internal.hpp"

#include <algorithm>
#include <cmath>

namespace icss::viewer_gui {

float map_axis(int value, int world_limit, int screen_origin, int screen_extent) {
    if (world_limit <= 1) {
        return static_cast<float>(screen_origin);
    }
    const int clamped = std::clamp(value, 0, world_limit - 1);
    return static_cast<float>(screen_origin)
        + (static_cast<float>(clamped) * static_cast<float>(screen_extent - 1) / static_cast<float>(world_limit - 1));
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

void draw_emphasized_line(SDL_Renderer* renderer,
                          SDL_Color color,
                          int x1,
                          int y1,
                          int x2,
                          int y2) {
    SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);
    SDL_RenderDrawLine(renderer, x1, y1, x2, y2);
    SDL_RenderDrawLine(renderer, x1 - 1, y1, x2 - 1, y2);
    SDL_RenderDrawLine(renderer, x1 + 1, y1, x2 + 1, y2);
    SDL_RenderDrawLine(renderer, x1, y1 - 1, x2, y2 - 1);
    SDL_RenderDrawLine(renderer, x1, y1 + 1, x2, y2 + 1);
}

RenderContext make_render_context(const ViewerState& state,
                                  const ViewerOptions& options,
                                  const GuiLayout& layout,
                                  TTF_Font* title_font,
                                  TTF_Font* body_font) {
    const auto phase_title = phase_banner_label(state.snapshot.phase);
    const auto phase_note = phase_operator_note(state.snapshot.phase);
    const auto phase_color = phase_accent(state.snapshot.phase);
    const auto authoritative_badge = authoritative_badge_label(state);
    const auto authoritative_color = authoritative_badge_color(state);
    const auto resilience_note = resilience_summary(state);
    const auto recommended_control = recommended_control_label(state);
    const int title_font_h = TTF_FontHeight(title_font);
    const int body_font_h = TTF_FontHeight(body_font);
    constexpr int header_text_gap = 4;
    const int header_text_total_h = title_font_h + header_text_gap + body_font_h;
    const int header_text_y = layout.header_panel.y + std::max(0, (layout.header_panel.h - header_text_total_h) / 2);
    return {
        state,
        options,
        layout,
        title_font,
        body_font,
        phase_title,
        phase_note,
        phase_color,
        authoritative_badge,
        authoritative_color,
        resilience_note,
        recommended_control,
        title_font_h,
        body_font_h,
        header_text_gap,
        header_text_total_h,
        header_text_y,
    };
}

void render_header_panel(SDL_Renderer* renderer, const RenderContext& ctx) {
    const auto& header_panel = ctx.layout.header_panel;
    const auto& phase_strip = ctx.layout.phase_strip;
    fill_panel(renderer, header_panel, rgba(18, 24, 32), rgba(56, 66, 86));
    SDL_SetRenderDrawColor(renderer, ctx.phase_color.r, ctx.phase_color.g, ctx.phase_color.b, 255);
    SDL_RenderFillRect(renderer, &phase_strip);
    draw_text(renderer, ctx.title_font, phase_strip.x + 14, phase_strip.y + 8, rgba(10, 12, 16), ctx.phase_title);
    const SDL_Rect status_badge {header_panel.x + header_panel.w - 196, header_panel.y + (header_panel.h - 28) / 2, 184, 28};
    fill_panel(renderer, status_badge, rgba(28, 34, 46), ctx.authoritative_color);
    draw_text(renderer,
              ctx.body_font,
              status_badge.x + 10,
              status_badge.y + std::max(0, (status_badge.h - ctx.body_font_h) / 2),
              ctx.authoritative_color,
              ctx.authoritative_badge,
              status_badge.w - 20);
    draw_text(renderer, ctx.title_font, header_panel.x + 360, ctx.header_text_y, rgba(236, 239, 244), "ICSS Tactical Viewer");
    draw_text(renderer,
              ctx.body_font,
              header_panel.x + 360,
              ctx.header_text_y + ctx.title_font_h + ctx.header_text_gap,
              rgba(200, 208, 220),
              ctx.phase_note,
              status_badge.x - (header_panel.x + 372));
}

}  // namespace icss::viewer_gui
