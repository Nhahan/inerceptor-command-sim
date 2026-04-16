# Scenario Design

## Baseline Scenario

A single representative training/control scenario is enough for the 4-week baseline.

### Narrative Shape
1. session starts
2. target appears / is detected
3. tracking request is issued
4. asset becomes available / activated
5. command is issued from the command console
6. server validates and judges the outcome
7. result is reviewed through AAR/replay

## Required Scenario Properties

- shows command -> validation -> state transition -> judgment
- shows multi-client visibility
- can generate meaningful AAR output
- can survive at least one abnormal network case in demo/test evidence

## Suggested State Flow

`Initialized -> Detecting -> Tracking -> AssetReady -> CommandIssued -> Engaging -> Judged -> Archived`

## Main Entity Types

- `Target`
- `Asset`
- `Track`
- `Session`
- `Command`
- `Judgment`

## Viewer Requirements Tied to Scenario

The tactical viewer should make the scenario understandable without game-like controls.
It should show:
- where targets/assets are
- whether tracking exists
- whether connection/snapshot status is healthy
- where the AAR playback cursor is in replay mode

## Non-Goals

- realistic combat doctrine
- detailed missile guidance or ballistics
- multiple mission packs
- complex enemy behavior trees
