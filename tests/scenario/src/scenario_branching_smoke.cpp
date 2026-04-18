#include <cassert>

#include "icss/core/config.hpp"
#include "icss/core/simulation.hpp"

int main() {
    using namespace icss::core;

    ScenarioConfig scenario;
    scenario.world_width = 576;
    scenario.world_height = 384;
    scenario.target_start_x = 80;
    scenario.target_start_y = 300;
    scenario.target_velocity_x = 5;
    scenario.target_velocity_y = -3;
    scenario.interceptor_start_x = 160;
    scenario.interceptor_start_y = 60;
    scenario.interceptor_speed_per_tick = 32;
    scenario.intercept_radius = 24;
    scenario.engagement_timeout_ticks = 60;

    SimulationSession early(1001U, 20, 200, scenario);
    early.connect_client(ClientRole::CommandConsole, 101U);
    early.connect_client(ClientRole::TacticalViewer, 201U);
    assert(early.start_scenario().accepted);
    assert(early.request_track().accepted);
    early.advance_tick();
    assert(early.activate_asset().accepted);
    assert(early.issue_command().accepted);
    for (int i = 0; i < scenario.engagement_timeout_ticks && !early.latest_snapshot().judgment.ready; ++i) {
        early.advance_tick();
    }
    assert(early.latest_snapshot().judgment.ready);
    assert(early.latest_snapshot().judgment.code == JudgmentCode::InterceptSuccess);
    assert(early.latest_snapshot().predicted_intercept_valid);
    assert(early.latest_snapshot().time_to_intercept_s > 0.0F);
    assert(early.latest_snapshot().asset_world_position.x != static_cast<float>(early.latest_snapshot().asset.position.x)
           || early.latest_snapshot().asset_world_position.y != static_cast<float>(early.latest_snapshot().asset.position.y));

    ScenarioConfig late_scenario = scenario;
    late_scenario.target_velocity_x = 10;
    late_scenario.target_velocity_y = -6;
    late_scenario.interceptor_speed_per_tick = 6;
    late_scenario.intercept_radius = 10;
    late_scenario.engagement_timeout_ticks = 6;

    SimulationSession late(1002U, 20, 200, late_scenario);
    late.connect_client(ClientRole::CommandConsole, 101U);
    late.connect_client(ClientRole::TacticalViewer, 201U);
    assert(late.start_scenario().accepted);
    assert(late.request_track().accepted);
    for (int i = 0; i < 6; ++i) {
        late.advance_tick();
    }
    assert(late.activate_asset().accepted);
    assert(late.issue_command().accepted);
    for (int i = 0; i < late_scenario.engagement_timeout_ticks && !late.latest_snapshot().judgment.ready; ++i) {
        late.advance_tick();
    }
    assert(late.latest_snapshot().judgment.ready);
    assert(late.latest_snapshot().judgment.code == JudgmentCode::TimeoutObserved);
    assert(late.latest_snapshot().predicted_intercept_valid);
    return 0;
}
