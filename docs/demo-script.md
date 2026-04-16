# Demo Script (3–5 minutes)

## Goal

Show command/control flow, real-time state propagation, one resilience case, and AAR evidence.

## Outline

### 0:00–0:20 — Project framing
- what this system is
- why it is not a consumer game

### 0:20–0:50 — Components
- server
- command console
- 2D tactical viewer

### 0:50–1:40 — Scenario start
- start session
- detect/track target
- show positions/status on viewer

### 1:40–2:30 — Command and judgment
- issue command from console
- show server-side validation/judgment result
- keep telemetry visible

### 2:30–3:20 — Resilience case
- demonstrate reconnect / timeout / UDP loss convergence
- explain what the viewer and logs show

### 3:20–4:20 — AAR
- open replay/AAR artifact
- point to timeline, event log, and judgment evidence

### Closing
- reiterate why the architecture was chosen
- reiterate what was intentionally excluded
