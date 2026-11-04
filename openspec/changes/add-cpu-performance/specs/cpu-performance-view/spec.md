# Spec Delta

## Purpose

Defines what the Performance › CPU screen of the frontend shows and how it refreshes. Layout and styling
follow the Figma design linked in `docs/ui-design/figma.onl`.

## ADDED Requirements

### Requirement: CPU screen shows backend data
The CPU screen SHALL display the values of `getCpuInfo` and `getCpuMetrics`: model and topology,
live gauges (usage, clock, temperature), the % Utilization chart (usage, clock, temperature over
60 s), the top CPU processes table, the summary (utilization, speed, temperature, processes, threads,
handles, up time, start time), the specifications, the CPU time breakdown with the User vs Kernel trend
and the scheduler activity. A `null` value SHALL be shown as `—`.

#### Scenario: Values on screen match the backend
- **WHEN** the CPU screen is open and a metrics response has been rendered
- **THEN** the summary utilization equals `current.usagePercent` rounded to an integer, followed by `%`
- **AND** the processes count equals `current.processes`

### Requirement: Refresh every second
The frontend SHALL request `getCpuMetrics` every 1000 ms and re-render the CPU screen, the CPU sidebar
entry and the CPU / uptime quick stats with each response. A failed request SHALL not stop the
refresh; the status bar SHALL show that the backend is unreachable until a request succeeds again.

#### Scenario: Live update
- **WHEN** the CPU screen stays open for 3 seconds
- **THEN** the up time shown in the summary changes

### Requirement: Shell shows the machine identity
The shell header SHALL show `hostName`, `osName`, `osVersion` and `userName` from `getSystemInfo`; the
status bar SHALL show the backend connection state, the update speed and the boot time.

#### Scenario: Header on Linux
- **WHEN** the app runs on Linux
- **THEN** the header shows the host name returned by `getSystemInfo`

### Requirement: Overview and per-core sub-tabs
The Overview and Per-core sub-tabs SHALL switch the CPU detail panel between the overview cards and a
grid with one usage chart per logical processor.

#### Scenario: Open the per-core grid
- **WHEN** the user clicks the Per-core sub-tab
- **THEN** one chart per element of `current.perCoreUsagePercent` is shown and the Per-core tab is active

### Requirement: CPU screen is the default screen
The app SHALL open on the CPU screen; the toggle button SHALL switch to the hello screen and back.

#### Scenario: Toggle to hello
- **WHEN** the user clicks the toggle button on the CPU screen
- **THEN** the hello screen shows the greeting returned by `getSampleHello`
