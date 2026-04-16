# Test Report

## Verification Goal

Prove that the portfolio demonstrates the intended server/system software qualities, not just a scripted happy path.

## Canonical Commands

### Configure

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
```

### Build

```bash
cmake --build build
```

### Test

```bash
ctest --test-dir build --output-on-failure
```

## Current Verification Scope

At the current stage, verification proves:
- the CMake project configures successfully
- the baseline executables build successfully
- the shared protocol schema compiles and passes smoke checks
- the baseline scenario flow passes end-to-end regression
- invalid command ordering is rejected and logged
- resilience/replay rendering behavior passes smoke verification

## Latest Verified Result

- configure: passed
- build: passed
- test: passed (`4/4` tests)
- runtime smoke: passed (`icss_server`, `icss_command_console`, `icss_tactical_viewer`)
- verified targets:
  - `protocol_smoke`
  - `scenario_flow`
  - `validation_rejects_invalid_flow`
  - `resilience_smoke`

## Acceptance Checks

### 1. Scenario Completeness
- [ ] scenario runs from start to AAR
- [ ] command validation occurs on server
- [ ] final judgment is produced by server

### 2. Multi-Client Evidence
- [ ] command console participates
- [ ] tactical viewer receives state/telemetry updates
- [ ] roles are meaningfully separated

### 3. Protocol Evidence
- [ ] TCP responsibilities demonstrated
- [ ] UDP snapshot/telemetry responsibilities demonstrated
- [ ] transport split documented clearly

### 4. Resilience Evidence
- [ ] at least one abnormal case is demonstrated
- [ ] logs show the abnormal case clearly
- [ ] system behavior after the abnormal case is understandable

### 5. AAR Evidence
- [ ] one sample session can be replayed/reconstructed
- [ ] event timeline includes key command/judgment points

### 6. Operational Evidence
- [ ] config separation exists
- [ ] logs have meaningful structure
- [ ] session cleanup/end-state is visible

## Evidence Links

- demo video: pending
- sample AAR: `assets/sample-aar/session-summary.md`
- logs: pending
- screenshots: pending
- protocol doc: `docs/protocol.md`
- concrete build/test commands: see canonical commands above
