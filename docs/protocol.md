# Protocol Design

## Design Goal

Keep the protocol small, explainable, and aligned with the portfolio's message: **reliable command handling + timely state propagation**.

## Transport Split

| Transport | Use | Why |
|---|---|---|
| TCP | session control, command submission, acknowledgements, critical judgment events | reliability and ordering matter |
| UDP | state snapshots, telemetry, non-critical frequent updates | freshness matters more than per-packet reliability |

## TCP Message Families

- session create/join/leave
- scenario start/stop
- track request
- asset activation
- command issue
- command validation result / acknowledgement
- critical judgment event
- replay/AAR request

## UDP Message Families

- periodic world snapshot
- target/asset state summary
- tracking status summary
- telemetry
  - tick
  - latency
  - packet loss estimate
  - last snapshot timestamp

## Baseline Reliability Strategy

### TCP
- strict request/response or event acknowledgement semantics for critical operations

### UDP
- latest snapshot wins
- sequence number and snapshot timestamp included
- no complex retransmission requirement in baseline
- recovery target: converge on the next valid snapshot

## Resilience Path to Implement

At least one of the following must be fully demonstrated:
- reconnect + resync
- timeout handling
- UDP snapshot loss convergence

## Packet/Message Design Guidelines

- include message type explicitly
- include session identifier
- include monotonic sequence/timestamp where useful
- keep operator intent separate from rendered world state
- make event payloads AAR-friendly

## Interview Explanation Targets

Be able to explain:
1. why not everything is TCP
2. why the viewer does not own truth
3. why snapshot convergence is enough for the baseline
4. how protocol structure helps replay/AAR
