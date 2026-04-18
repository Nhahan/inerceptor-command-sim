# Tactical Viewer GUI

Role:
- live window-based tactical viewer
- attach to the `socket_live` UDP path
- render read-only state, telemetry, and local viewer events

Guardrails:
- remains observability-first
- does not issue commands
- avoids game-style control loops or effects

Entrypoint:
- `clients/tactical-viewer-gui/src/main.cpp`

Current behavior:
- opens an SDL window
- sends UDP `session_join` and heartbeat traffic
- receives `world_snapshot` and `telemetry`
- renders the tactical picture plus a mission phase rail, an `Authoritative Status` panel, resilience/telemetry panel, and terminal-style server log
- shows a compact authoritative status badge (`ENGAGING`, `REJECTED`, `JUDGED`, `LIVE`, etc.) in the header instead of burying long status text in panel body content
- shows server event history in a larger terminal-style log panel
- keeps heartbeat traffic out of the event log and reports it as telemetry instead
- auto-attaches the control channel when the first control button is used
- provides `Start`, `Track`, `Activate`, `Command`, `Stop`, `Reset`, and a separate `Review` action
- treats review as post-judgment/post-archive AAR inspection rather than a primary live-control step
- uses the bottom panel as an event timeline: live server events plus control acknowledgements during operation, and a separate review mode after `Review`
- keeps the event timeline in a fixed terminal-style color scheme so mode changes do not re-style the panel
- highlights the next recommended control based on the current authoritative phase instead of relying on a static workflow hint
- exposes direct scenario setup controls for target/interceptor start positions, target velocity, interceptor speed, and timeout before the next `Start`
- keeps scenario setup changes scoped to the next `Start`; editing setup during a live run does not rewrite the active tactical picture
- renders a denser 576x384 world-space picture instead of the earlier toy-sized grid
- computes tracking confidence from noisy observations, covariance, measurement age, and missed updates instead of a fixed percentage
- automatically reissues `session_join` after stale telemetry so the viewer can recover from a live timeout without restarting the process
- accepts `--repo-root` so the viewer's setup panel preloads the same scenario config the server will run
- supports reset-to-initialized followed by a fresh `Start`
- gives immediate visual command feedback by highlighting the intercept line/target box and updating control state locally on accepted actions
- keeps the state-machine story explicit with `INITIALIZED -> DETECTING -> TRACKING -> INTERCEPTOR READY -> COMMAND ACCEPTED -> ENGAGING -> JUDGMENT PRODUCED -> ARCHIVED`
- supports `--headless --dump-state` for automated smoke coverage
