# Interceptor Command Simulation System

> C++ real-time simulation server with multi-client command/control, UDP/TCP state propagation, and AAR-focused observability.

## Project Overview

Interceptor Command Simulation System is a **single flagship portfolio project** for a LIG넥스원-oriented application.

The goal is **not** to present a consumer game. The goal is to present a **battlefield-system style real-time simulation/control system** that demonstrates:
- C++ real-time server/software engineering
- server-authoritative state management
- TCP/UDP role separation
- multi-client handling
- resilience under abnormal network conditions
- replayable logging / AAR (After Action Review)
- operability and explanation quality suitable for interviews

## Positioning

- **Primary positioning:** server SW / real-time systems SW
- **Secondary message:** adjacent fit for C4/ISR/unmanned-style systems through state management, command handling, situation propagation, and operability thinking

## Locked 4-Week Baseline

### Must Implement
- 1 C++ server
- 2 clients
  - command console
  - minimal 2D tactical viewer
- 1 representative scenario
- server-authoritative validation and judgment
- TCP/UDP split with documented responsibilities
- replayable event logging / AAR
- at least 1 resilience case
  - reconnect, timeout, or UDP loss recovery

### Must Show in the 2D Tactical Viewer
- target / asset position icons
- tracking status
- connection status
- tick / latency / packet loss telemetry
- last snapshot timestamp
- event log panel
- AAR playback cursor

### Explicit Non-Goals
- flashy effects or game-like presentation
- direct action controls such as WASD/manual aiming
- progression systems (items, ranking, leveling)
- precise real-world weapons physics or tactics
- multi-project portfolio spread

## Deliverables
- GitHub repository
- 3–5 minute demo video
- core documentation set under `docs/`
- sample AAR evidence under `assets/sample-aar/`

## Repository Guide

- `docs/architecture.md` — component boundaries and state authority
- `docs/protocol.md` — TCP/UDP responsibilities and message families
- `docs/scenario.md` — representative scenario and state flow
- `docs/operations.md` — logging, configuration, resilience, cleanup
- `docs/aar.md` — event schema and replay/AAR design
- `docs/test-report.md` — verification plan and evidence ledger
- `docs/interview-guide.md` — interview-facing messaging and Q&A
- `docs/demo-script.md` — recommended demo flow
- `docs/week1-checklist.md` — immediate Week 1 execution checklist

## High-Level Repo Layout

```text
server/
clients/
  command-console/
  tactical-viewer/
docs/
configs/
tests/
  scenario/
  protocol/
  resilience/
assets/
  diagrams/
  screenshots/
  sample-aar/
examples/
```

## Immediate Next Step

Start with `docs/week1-checklist.md` and lock:
1. component boundaries
2. protocol categories
3. event schema
4. scenario timeline
5. tactical viewer telemetry contract
