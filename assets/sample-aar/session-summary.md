# Sample AAR Session Summary

## Summary

- schema_version: icss-session-summary-v1
- session_id: 1001
- final_phase: archived
- snapshot_count: 8
- event_count: 12
- command_console_connection: connected
- viewer_connection: connected
- judgment_ready: true
- judgment_code: intercept_success
- last_event_type: session_ended
- resilience_case: reconnect_and_resync,udp_snapshot_gap_convergence
- latest_snapshot_sequence: 8
- latest_viewer_connection: connected
- latest_freshness: fresh

## Recent Events

- [tick 1] Command accepted (command_accepted)
- [tick 2] Snapshot gap exercised (resilience_triggered)
- [tick 3] Judgment produced (judgment_produced)
- [tick 3] Session archived (session_ended)
