#include "render_internal.hpp"

#include <deque>
#include <vector>

namespace icss::viewer_gui {
namespace {

constexpr int kReferenceStep = 48;

void draw_history(SDL_Renderer* renderer,
                  const RenderContext& ctx,
                  const std::deque<icss::core::Vec2>& history,
                  SDL_Color color) {
    if (history.size() < 2) {
        return;
    }
    std::vector<SDL_Point> points;
    points.reserve(history.size());
    for (const auto& pos : history) {
        points.push_back(SDL_Point {
            static_cast<int>(map_axis(pos.x, ctx.state.snapshot.world_width, ctx.layout.map_rect.x, ctx.layout.map_rect.w)),
            static_cast<int>(map_axis(pos.y, ctx.state.snapshot.world_height, ctx.layout.map_rect.y, ctx.layout.map_rect.h)),
        });
    }
    SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, 150);
    SDL_RenderDrawLines(renderer, points.data(), static_cast<int>(points.size()));
}

void draw_entity(SDL_Renderer* renderer,
                 const RenderContext& ctx,
                 const icss::core::EntityState& entity,
                 SDL_Color color) {
    SDL_Rect rect {0, 0, 8, 8};
    const auto center = (&entity == &ctx.state.snapshot.target)
        ? map_point_for_entity(ctx.layout.map_rect,
                               ctx.state.snapshot.world_width,
                               ctx.state.snapshot.world_height,
                               ctx.state.snapshot.target_world_position)
        : map_point_for_entity(ctx.layout.map_rect,
                               ctx.state.snapshot.world_width,
                               ctx.state.snapshot.world_height,
                               ctx.state.snapshot.asset_world_position);
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
}

}  // namespace

void render_map_panel(SDL_Renderer* renderer, const RenderContext& ctx) {
    const auto& map_rect = ctx.layout.map_rect;
    fill_panel(renderer, map_rect, rgba(36, 41, 52), rgba(74, 80, 96));
    SDL_SetRenderDrawColor(renderer, 74, 80, 96, 255);
    for (int x = 0; x <= map_rect.w; x += kReferenceStep) {
        const int px = map_rect.x + x;
        SDL_RenderDrawLine(renderer, px, map_rect.y, px, map_rect.y + map_rect.h);
    }
    for (int y = 0; y <= map_rect.h; y += kReferenceStep) {
        const int py = map_rect.y + y;
        SDL_RenderDrawLine(renderer, map_rect.x, py, map_rect.x + map_rect.w, py);
    }
    for (int x = 0; x <= ctx.state.snapshot.world_width; x += 96) {
        const int px = static_cast<int>(map_axis(x, ctx.state.snapshot.world_width, map_rect.x, map_rect.w));
        draw_text(renderer, ctx.body_font, px + 2, map_rect.y - 22, rgba(140, 149, 168), std::to_string(x));
    }
    for (int y = 0; y <= ctx.state.snapshot.world_height; y += 96) {
        const int py = static_cast<int>(map_axis(y, ctx.state.snapshot.world_height, map_rect.y, map_rect.h));
        draw_text(renderer, ctx.body_font, map_rect.x - 24, py + 2, rgba(140, 149, 168), std::to_string(y));
    }

    draw_history(renderer, ctx, ctx.state.target_history, rgba(244, 67, 54, 160));
    draw_history(renderer, ctx, ctx.state.asset_history, rgba(66, 165, 245, 160));
    const auto target_center = map_point_for_entity(map_rect, ctx.state.snapshot.world_width, ctx.state.snapshot.world_height, ctx.state.snapshot.target_world_position);
    const auto asset_center = map_point_for_entity(map_rect, ctx.state.snapshot.world_width, ctx.state.snapshot.world_height, ctx.state.snapshot.asset_world_position);
    const bool target_motion_visible = target_motion_visual_visible(ctx.state);
    const bool asset_motion_visible = asset_motion_visual_visible(ctx.state);
    const bool engagement_live = engagement_visual_visible(ctx.state);

    if (target_motion_visible) {
        draw_emphasized_line(renderer,
                             rgba(244, 67, 54, 220),
                             static_cast<int>(target_center.x),
                             static_cast<int>(target_center.y),
                             static_cast<int>(target_center.x + ctx.state.snapshot.target_velocity.x * 8.0F),
                             static_cast<int>(target_center.y + ctx.state.snapshot.target_velocity.y * 8.0F));
        SDL_Rect lock_box {static_cast<int>(target_center.x) - 12, static_cast<int>(target_center.y) - 12, 24, 24};
        SDL_SetRenderDrawColor(renderer, 255, 214, 102, 220);
        SDL_RenderDrawRect(renderer, &lock_box);
        SDL_RenderDrawLine(renderer, lock_box.x, lock_box.y, lock_box.x + 6, lock_box.y);
        SDL_RenderDrawLine(renderer, lock_box.x, lock_box.y, lock_box.x, lock_box.y + 6);
        SDL_RenderDrawLine(renderer, lock_box.x + lock_box.w - 1, lock_box.y, lock_box.x + lock_box.w - 7, lock_box.y);
        SDL_RenderDrawLine(renderer, lock_box.x + lock_box.w - 1, lock_box.y, lock_box.x + lock_box.w - 1, lock_box.y + 6);
        SDL_RenderDrawLine(renderer, lock_box.x, lock_box.y + lock_box.h - 1, lock_box.x + 6, lock_box.y + lock_box.h - 1);
        SDL_RenderDrawLine(renderer, lock_box.x, lock_box.y + lock_box.h - 1, lock_box.x, lock_box.y + lock_box.h - 7);
        SDL_RenderDrawLine(renderer, lock_box.x + lock_box.w - 1, lock_box.y + lock_box.h - 1, lock_box.x + lock_box.w - 7, lock_box.y + lock_box.h - 1);
        SDL_RenderDrawLine(renderer, lock_box.x + lock_box.w - 1, lock_box.y + lock_box.h - 1, lock_box.x + lock_box.w - 1, lock_box.y + lock_box.h - 7);
        draw_text(renderer,
                  ctx.body_font,
                  static_cast<int>(target_center.x) + 16,
                  static_cast<int>(target_center.y) - 28,
                  rgba(255, 214, 102),
                  "TRACK LOCK " + std::to_string(ctx.state.snapshot.track.confidence_pct) + "%");
        draw_text(renderer,
                  ctx.body_font,
                  static_cast<int>(target_center.x) + 16,
                  static_cast<int>(target_center.y) - 10,
                  rgba(188, 198, 214),
                  "cov=" + format_fixed_1(ctx.state.snapshot.track.covariance_trace)
                      + " age=" + std::to_string(ctx.state.snapshot.track.measurement_age_ticks));
    }
    if (asset_motion_visible) {
        draw_emphasized_line(renderer,
                             rgba(66, 165, 245, 220),
                             static_cast<int>(asset_center.x),
                             static_cast<int>(asset_center.y),
                             static_cast<int>(asset_center.x + ctx.state.snapshot.asset_velocity.x * 8.0F),
                             static_cast<int>(asset_center.y + ctx.state.snapshot.asset_velocity.y * 8.0F));
    }
    if (engagement_live) {
        if (predicted_marker_visual_visible(ctx.state)) {
            const auto predicted_center = map_point_for_entity(
                map_rect,
                ctx.state.snapshot.world_width,
                ctx.state.snapshot.world_height,
                ctx.state.snapshot.predicted_intercept_position);
            draw_emphasized_line(renderer,
                                 rgba(182, 255, 182, 240),
                                 static_cast<int>(predicted_center.x) - 10,
                                 static_cast<int>(predicted_center.y),
                                 static_cast<int>(predicted_center.x) + 10,
                                 static_cast<int>(predicted_center.y));
            draw_emphasized_line(renderer,
                                 rgba(182, 255, 182, 240),
                                 static_cast<int>(predicted_center.x),
                                 static_cast<int>(predicted_center.y) - 10,
                                 static_cast<int>(predicted_center.x),
                                 static_cast<int>(predicted_center.y) + 10);
            SDL_Rect predicted_box {static_cast<int>(predicted_center.x) - 6, static_cast<int>(predicted_center.y) - 6, 12, 12};
            SDL_SetRenderDrawColor(renderer, 182, 255, 182, 180);
            SDL_RenderDrawRect(renderer, &predicted_box);
        }
        draw_emphasized_line(renderer,
                             rgba(255, 149, 0, 255),
                             static_cast<int>(asset_center.x),
                             static_cast<int>(asset_center.y),
                             static_cast<int>(target_center.x),
                             static_cast<int>(target_center.y));
        SDL_Rect target_box {static_cast<int>(target_center.x) - 10, static_cast<int>(target_center.y) - 10, 20, 20};
        SDL_RenderDrawRect(renderer, &target_box);
    }

    draw_entity(renderer, ctx, ctx.state.snapshot.target, rgba(244, 67, 54));
    draw_entity(renderer, ctx, ctx.state.snapshot.asset, rgba(66, 165, 245));
}

}  // namespace icss::viewer_gui
