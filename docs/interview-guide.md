# Interview Guide

## One-Sentence Positioning

I built a C++ server-authoritative simulation/control baseline to show protocol-defined transport separation, multi-client state propagation, resilience handling, and replayable AAR evidence in a battlefield-system style portfolio.

## What This Portfolio Is Trying to Prove

1. I can design a C++ real-time server/system, not just a toy game feature.
2. I understand why command reliability and state freshness need different transport strategies, even though the current baseline models those roles in-process rather than through live sockets.
3. I think about operability, traceability, and post-run analysis.

## Questions I Must Be Ready For

### Why is this not just a game server?
Because the system is framed around command validation, state authority, resilience, and AAR traceability rather than player action, game reward loops, or visual entertainment.

### Why server-authoritative?
To make the validation/judgment path explicit and explainable. The server is the only source of truth.

### Why TCP and UDP both?
TCP handles reliability-sensitive control flows. UDP handles fresh snapshot/telemetry flows.

### Why include AAR?
Because the portfolio should show not just execution, but also explainability and post-run traceability.

### Why use a minimal 2D viewer?
To make state propagation and telemetry legible without drifting into game-like UX.

## Expression Guardrail

Do **not** claim direct C4/ISR/unmanned product experience unless it is true.
Frame it as:
- primary strength: server SW / real-time systems SW
- adjacent fit: state management, command handling, observability, and control-system thinking
