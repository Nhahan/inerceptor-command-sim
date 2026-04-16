# Command Console Plan

Purpose:
- operator-facing command/control client
- submit commands over the reliability-sensitive path
- display acknowledgements and critical outcomes

Guardrails:
- sends requests; does not own authoritative state
- favors clarity of operator intent over rich UI behavior

Committed entrypoint:
- `clients/command-console/src/main.cpp`

Current baseline behavior:
- runs the canonical operator command order
- prints acceptance results from the authoritative runtime
