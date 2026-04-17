#pragma once

#include <filesystem>
#include <string_view>

#include "icss/core/config.hpp"
#include "icss/core/simulation.hpp"

namespace icss::core {

struct RuntimeResult {
    RuntimeConfig config;
    SessionSummary summary;
};

class BaselineRuntime {
public:
    explicit BaselineRuntime(RuntimeConfig config);

    [[nodiscard]] const RuntimeConfig& config() const;
    [[nodiscard]] RuntimeResult run() const;

private:
    RuntimeConfig config_;
};

RuntimeConfig default_runtime_config(const std::filesystem::path& repo_root);
SimulationSession run_baseline_demo(const RuntimeConfig& config);
void write_runtime_session_log(const RuntimeConfig& config,
                               std::string_view backend_name,
                               const SessionSummary& summary,
                               const std::vector<EventRecord>& events);

}  // namespace icss::core
