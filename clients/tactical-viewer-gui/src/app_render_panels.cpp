#include "render_internal.hpp"

namespace icss::viewer_gui {

void render_gui(SDL_Renderer* renderer,
                TTF_Font* title_font,
                TTF_Font* body_font,
                const ViewerState& state,
                const ViewerOptions& options) {
    const auto layout = build_layout(options);
    const auto ctx = make_render_context(state, options, layout, title_font, body_font);

    SDL_SetRenderDrawColor(renderer, 15, 18, 24, 255);
    SDL_RenderClear(renderer);
    render_header_panel(renderer, ctx);
    render_map_panel(renderer, ctx);
    render_phase_panel(renderer, ctx);
    render_decision_panel(renderer, ctx);
    render_resilience_panel(renderer, ctx);
    render_control_panel(renderer, ctx);
    render_setup_panel(renderer, ctx);
    render_entity_panel(renderer, ctx);
    render_event_panel(renderer, ctx);
    SDL_RenderPresent(renderer);
}

}  // namespace icss::viewer_gui
