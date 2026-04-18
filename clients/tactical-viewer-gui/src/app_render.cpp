#include "app.hpp"

#include <stdexcept>

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

}  // namespace icss::viewer_gui
