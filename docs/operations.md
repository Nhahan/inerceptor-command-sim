# Operations / Supportability

## Why This Exists

The portfolio should feel like an operable system, not a throwaway demo.

## Required Operational Concerns

### Configuration Separation
Provide separate config domains for:
- server/network
- scenario
- logging

### Logging
Minimum expectations:
- structured enough to reconstruct timeline
- clear severity levels
- session lifecycle events recorded
- command acceptance/rejection recorded
- resilience events recorded

### Session Cleanup
On shutdown or disconnect:
- release connection/session resources
- finalize pending logs
- mark session end reason

### Failure Handling
At minimum, make abnormal states visible for:
- timeout
- disconnect
- reconnect
- snapshot loss convergence

### Telemetry Visibility
The tactical viewer or command console should expose at least:
- connection status
- tick
- latency
- packet loss estimate
- last snapshot timestamp

## Demo/Interview Value

Operational evidence is part of the portfolio's message.
A reviewer should see that the system designer thought about:
- monitoring
- cleanup
- reproducibility
- traceability
- post-run analysis
