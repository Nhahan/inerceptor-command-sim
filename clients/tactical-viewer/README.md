# Tactical Viewer Plan

Purpose:
- minimal 2D tactical/state viewer
- render positions/status without game-like controls
- display telemetry and event panel
- support AAR playback cursor visualization

Required baseline elements:
- target / asset position icons
- tracking status
- connection status
- tick / latency / packet loss telemetry
- last snapshot timestamp
- event log panel
- AAR playback cursor

Non-goals:
- direct gameplay controls
- cinematic effects
- score / reward UI

Committed entrypoint:
- `clients/tactical-viewer/src/main.cpp`

Current baseline behavior:
- renders an ASCII tactical frame
- shows telemetry, connection state, recent events, and AAR cursor position
