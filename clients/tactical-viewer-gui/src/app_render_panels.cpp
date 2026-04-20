#include "render_internal.hpp"

namespace icss::viewer_gui {

void render_gui(SDL_Renderer* renderer,
                TTF_Font* title_font,
                TTF_Font* body_font,
                TTF_Font* compact_font,
                const ViewerState& state,
                const ViewerOptions& options) {
    const auto layout = build_layout(options, state);
    const auto ctx = make_render_context(state, options, layout, title_font, body_font, compact_font);

    SDL_SetRenderDrawColor(renderer, 15, 18, 24, 255);
    SDL_RenderClear(renderer);
    render_header_panel(renderer, ctx);
    fill_panel(renderer, layout.content_panel, rgba(11, 14, 20), rgba(48, 58, 76));
    fill_panel(renderer, layout.rail_panel, rgba(14, 18, 26), rgba(42, 50, 66));
    render_map_panel(renderer, ctx);
    render_decision_panel(renderer, ctx);
    if (layout.show_resilience_panel) {
        render_resilience_panel(renderer, ctx);
    }
    render_control_panel(renderer, ctx);
    if (layout.show_setup_panel) {
        render_setup_panel(renderer, ctx);
    }
    render_event_panel(renderer, ctx);
    SDL_RenderPresent(renderer);
}

}  // namespace icss::viewer_gui
