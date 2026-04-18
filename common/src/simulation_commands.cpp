#include "icss/core/simulation.hpp"

namespace icss::core {

CommandResult SimulationSession::start_scenario() {
    if (command_console_.connection == ConnectionState::Disconnected) {
        return reject_command("Scenario start rejected",
                              "command console must connect before starting the scenario");
    }
    if (phase_ != SessionPhase::Initialized) {
        return reject_command("Scenario start rejected",
                              "scenario can only be started from initialized state");
    }
    phase_ = SessionPhase::Detecting;
    target_.active = true;
    clear_track_state();
    asset_status_ = AssetStatus::Idle;
    command_status_ = CommandLifecycle::None;
    judgment_ = {};
    push_event(protocol::EventType::SessionStarted,
               "simulation_server",
               {target_.id},
               "Scenario started",
               "The authoritative loop is now in detecting state.");
    record_snapshot();
    return {true, "scenario started", protocol::TcpMessageKind::ScenarioStart};
}

void SimulationSession::configure_scenario(ScenarioConfig scenario) {
    scenario_ = std::move(scenario);
    reset_world_state_from_scenario();
    clear_track_state();
    asset_status_ = AssetStatus::Idle;
    command_status_ = CommandLifecycle::None;
    judgment_ = {};
    engagement_started_tick_.reset();
}

CommandResult SimulationSession::reset_session(std::string) {
    tick_ = 0;
    sequence_ = 0;
    clock_ms_ = 1'776'327'000'000ULL;
    phase_ = SessionPhase::Initialized;
    reset_world_state_from_scenario();
    clear_track_state();
    asset_status_ = AssetStatus::Idle;
    command_status_ = CommandLifecycle::None;
    judgment_ = {};
    engagement_started_tick_.reset();
    reconnect_exercised_ = false;
    timeout_exercised_ = false;
    packet_gap_exercised_ = false;
    events_.clear();
    snapshots_.clear();
    if (command_console_.connection == ConnectionState::Reconnected) {
        command_console_.connection = ConnectionState::Connected;
    }
    if (tactical_viewer_.connection == ConnectionState::Reconnected) {
        tactical_viewer_.connection = ConnectionState::Connected;
    }
    record_snapshot();
    return {true, "session reset to initialized state", protocol::TcpMessageKind::CommandAck};
}

CommandResult SimulationSession::request_track() {
    if (phase_ != SessionPhase::Detecting) {
        return reject_command("Track request rejected",
                              "track request is only valid while detecting",
                              {target_.id});
    }
    seed_track_state_from_target();
    phase_ = SessionPhase::Tracking;
    push_event(protocol::EventType::TrackUpdated,
               "command_console",
               {target_.id},
               "Track request accepted",
               "Target is now actively tracked by the server.");
    record_snapshot();
    return {true, "track accepted", protocol::TcpMessageKind::TrackRequest};
}

CommandResult SimulationSession::activate_asset() {
    if (phase_ != SessionPhase::Tracking) {
        return reject_command("Interceptor activation rejected",
                              "interceptor activation is only valid while tracking",
                              {asset_.id});
    }
    if (!track_.active) {
        return reject_command("Interceptor activation rejected",
                              "interceptor activation requires an active track",
                              {asset_.id});
    }
    asset_status_ = AssetStatus::Ready;
    asset_.active = true;
    phase_ = SessionPhase::AssetReady;
    push_event(protocol::EventType::AssetUpdated,
               "command_console",
               {asset_.id},
               "Interceptor activation accepted",
               "Interceptor is ready for command issue.");
    record_snapshot();
    return {true, "interceptor ready", protocol::TcpMessageKind::AssetActivate};
}

CommandResult SimulationSession::issue_command() {
    if (phase_ != SessionPhase::AssetReady) {
        return reject_command("Command rejected",
                              "command issue is only valid while asset_ready",
                              {asset_.id, target_.id});
    }
    if (asset_status_ != AssetStatus::Ready) {
        return reject_command("Command rejected",
                              "command issue requires asset_ready state",
                              {asset_.id, target_.id});
    }
    command_status_ = CommandLifecycle::Accepted;
    phase_ = SessionPhase::CommandIssued;
    engagement_started_tick_ = tick_;
    push_event(protocol::EventType::CommandAccepted,
               "command_console",
               {asset_.id, target_.id},
               "Command accepted",
               "Authoritative server accepted the operator command for judgment.");
    record_snapshot();
    return {true, "command accepted", protocol::TcpMessageKind::CommandAck};
}

CommandResult SimulationSession::record_transport_rejection(std::string summary,
                                                            std::string reason,
                                                            std::vector<std::string> entity_ids) {
    return reject_command(std::move(summary), std::move(reason), std::move(entity_ids));
}

void SimulationSession::archive_session() {
    phase_ = SessionPhase::Archived;
    push_event(protocol::EventType::SessionEnded,
               "simulation_server",
               {target_.id, asset_.id},
               "Session archived",
               "Scenario completed and artifacts are ready for AAR review.");
    record_snapshot();
}

}  // namespace icss::core
