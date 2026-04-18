#include "icss/core/simulation.hpp"

namespace icss::core {

void SimulationSession::connect_client(ClientRole role, std::uint32_t sender_id) {
    auto& state = client(role);
    const auto reconnecting = state.has_connected_before;
    state.sender_id = sender_id;
    state.has_connected_before = true;
    state.connection = reconnecting ? ConnectionState::Reconnected : ConnectionState::Connected;
    if (reconnecting) {
        reconnect_exercised_ = true;
        push_event(protocol::EventType::ClientReconnected,
                   to_string(role),
                   {},
                   "Client reconnected",
                   "Client resumed control-plane visibility and should resync on the next snapshot.");
        push_event(protocol::EventType::ResilienceTriggered,
                   "simulation_server",
                   {},
                   "Reconnect/resync path exercised",
                   "The baseline resilience case uses reconnect + latest-snapshot convergence.");
    } else {
        push_event(protocol::EventType::ClientJoined,
                   to_string(role),
                   {},
                   "Client joined session",
                   "Client registered before scenario activity.");
    }
}

void SimulationSession::disconnect_client(ClientRole role, std::string reason) {
    auto& state = client(role);
    state.connection = ConnectionState::Disconnected;
    push_event(protocol::EventType::ClientLeft,
               to_string(role),
               {},
               "Client disconnected",
               std::move(reason));
}

void SimulationSession::timeout_client(ClientRole role, std::string reason) {
    auto& state = client(role);
    state.connection = ConnectionState::TimedOut;
    timeout_exercised_ = true;
    push_event(protocol::EventType::ResilienceTriggered,
               "simulation_server",
               {},
               "Client timeout exercised",
               std::move(reason));
    record_snapshot();
}

}  // namespace icss::core
