#include <cassert>
#include <filesystem>
#include <fstream>
#include <sstream>

#include "icss/core/runtime.hpp"
#include "tests/support/temp_repo.hpp"

int main() {
    namespace fs = std::filesystem;
    using namespace icss::core;

    const fs::path temp_root = icss::testsupport::make_temp_configured_repo("icss_runtime_artifacts_test_");

    BaselineRuntime runtime(default_runtime_config(temp_root));
    const auto result = runtime.run();

    assert(result.summary.judgment_ready);
    assert(fs::exists(temp_root / "assets/sample-aar/replay-timeline.json"));
    assert(fs::exists(temp_root / "assets/sample-aar/session-summary.md"));
    assert(fs::exists(temp_root / "examples/sample-output.md"));
    assert(fs::exists(temp_root / "logs/session.log"));

    std::ifstream summary_in(temp_root / "assets/sample-aar/session-summary.md");
    std::stringstream summary_buffer;
    summary_buffer << summary_in.rdbuf();
    const auto summary_text = summary_buffer.str();
    assert(summary_text.find("latest_snapshot_sequence:") != std::string::npos);
    assert(summary_text.find("latest_freshness:") != std::string::npos);
    assert(summary_text.find("## Recent Events") != std::string::npos);

    std::ifstream output_in(temp_root / "examples/sample-output.md");
    std::stringstream output_buffer;
    output_buffer << output_in.rdbuf();
    const auto output_text = output_buffer.str();
    assert(output_text.find("backend: in_process") != std::string::npos);
    assert(output_text.find("latest_freshness:") != std::string::npos);
    assert(output_text.find("latest_snapshot_sequence:") != std::string::npos);
    assert(output_text.find("freshness=") != std::string::npos);

    fs::remove_all(temp_root);
    return 0;
}
