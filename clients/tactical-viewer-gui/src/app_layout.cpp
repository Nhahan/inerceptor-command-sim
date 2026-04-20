#include "app.hpp"

#include <array>
#include <stdexcept>

namespace icss::viewer_gui {
namespace {

bool rects_overlap(const SDL_Rect& lhs, const SDL_Rect& rhs) {
    return lhs.x < rhs.x + rhs.w
        && lhs.x + lhs.w > rhs.x
        && lhs.y < rhs.y + rhs.h
        && lhs.y + lhs.h > rhs.y;
}

std::vector<std::string> visible_live_actions(LayoutMode mode) {
    switch (mode) {
    case LayoutMode::StandbySetup:
        return {"Start"};
    case LayoutMode::LiveTactical:
        return {"Track", "Ready", "Engage", "Reset"};
    case LayoutMode::ReviewTactical:
        return {"AAR", "Reset"};
    }
    return {"Start"};
}

std::vector<Button> make_live_control_buttons(const SDL_Rect& control_panel, LayoutMode mode) {
    const int button_gap = 10;
    const int button_h = 30;
    const auto actions = visible_live_actions(mode);
    std::vector<Button> buttons;
    if (actions.empty()) {
        return buttons;
    }

    if (actions.size() == 1) {
        buttons.push_back({actions.front(), std::string(actions.front()), {control_panel.x + 12, control_panel.y + 102, control_panel.w - 24, button_h}});
        return buttons;
    }

    if (mode == LayoutMode::ReviewTactical && actions.size() == 2) {
        const int row_y = control_panel.y + 102;
        const int button_w = (control_panel.w - 24 - button_gap) / 2;
        buttons.push_back({actions[0], std::string(actions[0]), {control_panel.x + 12, row_y, button_w, button_h}});
        buttons.push_back({actions[1], std::string(actions[1]), {control_panel.x + 12 + button_w + button_gap, row_y, button_w, button_h}});
        return buttons;
    }

    const int row1_y = control_panel.y + 102;
    const int row2_y = control_panel.y + 138;
    const int button_w = (control_panel.w - 24 - (button_gap * 2)) / 3;
    for (std::size_t index = 0; index < actions.size(); ++index) {
        const int row = static_cast<int>(index / 3);
        const int col = static_cast<int>(index % 3);
        const int y = row == 0 ? row1_y : row2_y;
        buttons.push_back({
            actions[index],
            std::string(actions[index]),
            {control_panel.x + 12 + col * (button_w + button_gap), y, button_w, button_h},
        });
    }
    return buttons;
}

std::vector<Button> make_setup_buttons(const SDL_Rect& setup_panel) {
    const int row1_y = setup_panel.y + 60;
    const int row2_y = setup_panel.y + 86;
    const int row3_y = setup_panel.y + 112;
    const int row4_y = setup_panel.y + 138;
    const int button_w = 28;
    const int button_h = 18;
    const int button_gap = 4;

    auto row_buttons = [&](int row_y,
                           std::string_view x_dec,
                           std::string_view x_inc,
                           std::string_view y_dec,
                           std::string_view y_inc,
                           std::string_view x_dec_label,
                           std::string_view x_inc_label,
                           std::string_view y_dec_label,
                           std::string_view y_inc_label) {
        const int start_x = setup_panel.x + setup_panel.w - 12 - ((button_w * 4) + (button_gap * 3));
        return std::array<Button, 4>{
            Button{std::string(x_dec), std::string(x_dec_label), {start_x + 0 * (button_w + button_gap), row_y + 4, button_w, button_h}},
            Button{std::string(x_inc), std::string(x_inc_label), {start_x + 1 * (button_w + button_gap), row_y + 4, button_w, button_h}},
            Button{std::string(y_dec), std::string(y_dec_label), {start_x + 2 * (button_w + button_gap), row_y + 4, button_w, button_h}},
            Button{std::string(y_inc), std::string(y_inc_label), {start_x + 3 * (button_w + button_gap), row_y + 4, button_w, button_h}},
        };
    };

    std::vector<Button> buttons;
    const auto target_pos = row_buttons(row1_y, "target_pos_x_dec", "target_pos_x_inc", "target_pos_y_dec", "target_pos_y_inc", "X-", "X+", "Y-", "Y+");
    const auto target_vel = row_buttons(row2_y, "target_vel_x_dec", "target_vel_x_inc", "target_vel_y_dec", "target_vel_y_inc", "X-", "X+", "Y-", "Y+");
    const auto angle_buttons = std::array<Button, 2>{
        Button{"launch_angle_dec", "-A", {setup_panel.x + setup_panel.w - 12 - ((button_w * 2) + button_gap), row3_y + 4, button_w, button_h}},
        Button{"launch_angle_inc", "+A", {setup_panel.x + setup_panel.w - 12 - button_w, row3_y + 4, button_w, button_h}},
    };
    const auto asset_dyn = row_buttons(row4_y, "interceptor_speed_dec", "interceptor_speed_inc", "timeout_dec", "timeout_inc", "S-", "S+", "T-", "T+");
    buttons.insert(buttons.end(), target_pos.begin(), target_pos.end());
    buttons.insert(buttons.end(), target_vel.begin(), target_vel.end());
    buttons.insert(buttons.end(), angle_buttons.begin(), angle_buttons.end());
    buttons.insert(buttons.end(), asset_dyn.begin(), asset_dyn.end());
    return buttons;
}

}  // namespace

GuiLayout build_layout(const ViewerOptions& options, const ViewerState& state) {
    constexpr int margin = 24;
    constexpr int header_x = 24;
    constexpr int header_y = 18;
    constexpr int header_h = 64;
    constexpr int gap_after_header = 24;
    constexpr int column_gap = 18;
    constexpr int section_gap = 12;
    constexpr int gap_before_event_panel = 14;

    GuiLayout layout;
    layout.mode = layout_mode_for_state(state);
    layout.show_resilience_panel = layout.mode == LayoutMode::LiveTactical;
    layout.show_setup_panel = layout.mode == LayoutMode::StandbySetup;
    layout.header_panel = {header_x, header_y, options.width - (margin * 2), header_h};
    layout.phase_strip = {layout.header_panel.x + 12, layout.header_panel.y + 12, 330, layout.header_panel.h - 24};
    const int content_y = header_y + header_h + gap_after_header;
    const int rail_w = layout.mode == LayoutMode::StandbySetup ? 340 : 300;
    const int event_panel_h = layout.mode == LayoutMode::ReviewTactical ? 176
        : (layout.mode == LayoutMode::StandbySetup ? 146 : 118);
    const int upper_h = options.height - margin - content_y - gap_before_event_panel - event_panel_h;
    layout.content_panel = {margin, content_y, options.width - (margin * 2), upper_h + gap_before_event_panel + event_panel_h};
    const int map_w = options.width - (margin * 2) - column_gap - rail_w;
    layout.map_rect = {margin, content_y, map_w, upper_h};
    layout.entity_panel = {};
    layout.phase_panel = {};
    layout.panel_x = layout.map_rect.x + layout.map_rect.w + column_gap;
    layout.rail_panel = {layout.panel_x - 10, content_y, options.width - margin - (layout.panel_x - 10), layout.content_panel.h};
    const int decision_h = layout.mode == LayoutMode::ReviewTactical ? 150 : 144;
    const int control_h = layout.mode == LayoutMode::ReviewTactical ? 154
        : (layout.mode == LayoutMode::LiveTactical ? 180 : 140);
    layout.decision_panel = {layout.panel_x, content_y, rail_w, decision_h};
    layout.control_panel = {layout.panel_x, layout.decision_panel.y + layout.decision_panel.h + section_gap, rail_w, control_h};

    const int lower_panel_y = layout.control_panel.y + layout.control_panel.h + section_gap;
    const int lower_panel_h = std::max(0, (content_y + layout.content_panel.h) - lower_panel_y);
    if (layout.show_resilience_panel) {
        layout.resilience_panel = {layout.panel_x, lower_panel_y, rail_w, std::min(lower_panel_h, 148)};
    } else {
        layout.resilience_panel = {};
    }
    if (layout.show_setup_panel) {
        layout.setup_panel = {layout.panel_x, lower_panel_y, rail_w, lower_panel_h};
    } else {
        layout.setup_panel = {};
    }
    layout.event_panel = {margin,
                          content_y + upper_h + gap_before_event_panel,
                          map_w,
                          event_panel_h};
    layout.buttons = make_live_control_buttons(layout.control_panel, layout.mode);
    if (layout.show_setup_panel) {
        const auto setup_buttons = make_setup_buttons(layout.setup_panel);
        layout.buttons.insert(layout.buttons.end(), setup_buttons.begin(), setup_buttons.end());
    }

    std::vector<SDL_Rect> panels;
    panels.reserve(7);
    auto append_visible = [&](const SDL_Rect& rect) {
        if (rect.w > 0 && rect.h > 0) {
            panels.push_back(rect);
        }
    };
    append_visible(layout.header_panel);
    append_visible(layout.map_rect);
    append_visible(layout.decision_panel);
    append_visible(layout.control_panel);
    append_visible(layout.resilience_panel);
    append_visible(layout.setup_panel);
    append_visible(layout.event_panel);
    for (std::size_t i = 0; i < panels.size(); ++i) {
        for (std::size_t j = i + 1; j < panels.size(); ++j) {
            if (rects_overlap(panels[i], panels[j])) {
                throw std::runtime_error("gui layout overlap detected");
            }
        }
    }
    for (std::size_t i = 0; i < layout.buttons.size(); ++i) {
        const auto& rect = layout.buttons[i].rect;
        const auto inside_control = rect.x >= layout.control_panel.x
            && rect.y >= layout.control_panel.y
            && rect.x + rect.w <= layout.control_panel.x + layout.control_panel.w
            && rect.y + rect.h <= layout.control_panel.y + layout.control_panel.h;
        const auto inside_setup = rect.x >= layout.setup_panel.x
            && rect.y >= layout.setup_panel.y
            && rect.x + rect.w <= layout.setup_panel.x + layout.setup_panel.w
            && rect.y + rect.h <= layout.setup_panel.y + layout.setup_panel.h;
        if (!inside_control && !inside_setup) {
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

}  // namespace icss::viewer_gui
