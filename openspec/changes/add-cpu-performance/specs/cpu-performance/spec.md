# Spec Delta

## Purpose

Serves the data of the Performance › CPU screen: static processor facts and live metrics measured by
the backend from the operating system (Windows, Linux).

## ADDED Requirements

### Requirement: CPU static information
`GET /api/v1/cpu/info` SHALL respond with status 200 and a `CpuInfo` body: `model`, `sockets`, `cores`,
`logicalProcessors`, `baseSpeedMhz`, `maxSpeedMhz`, `virtualization` (`"enabled"`, `"disabled"` or
`"unknown"`), `cache` (`l1Bytes`, `l2Bytes`, `l3Bytes`) and `countersSource`. A value the platform cannot
report SHALL be `null`; counts SHALL be at least 1.

#### Scenario: Info on a supported platform
- **WHEN** an authorized client sends `GET /api/v1/cpu/info` to a backend running on Windows or Linux
- **THEN** the response status is 200
- **AND** `logicalProcessors` is at least `cores`, and `cores` is at least `sockets`

### Requirement: Sampling every second
The backend SHALL sample the CPU counters every 1000 ms on a background thread, independently of
client requests, and SHALL keep the last 60 measured intervals.

#### Scenario: History grows to 60 samples
- **WHEN** the backend has been running for more than 61 seconds
- **THEN** every history array of `GET /api/v1/cpu/metrics` has exactly 60 elements, oldest first

### Requirement: CPU live metrics
`GET /api/v1/cpu/metrics` SHALL respond with status 200 and a `CpuMetrics` body holding the metrics of
the last measured interval and the history. Percentages SHALL be in [0, 100]; rates SHALL be per
second; a metric the platform cannot report SHALL be `null`.

The current metrics SHALL contain: total usage, per-core usage (one entry per logical processor),
current speed, temperature, process / thread / handle counts, uptime, boot time, the CPU time breakdown
(`user`, `kernel`, `dpc`, `interrupt`, `idle`, summing to 100 within rounding when all are present), the
scheduler rates (context switches, interrupts, system calls, DPCs) and the 10 processes with the
highest CPU usage in the interval, highest first.

Process CPU usage SHALL be expressed as a share of the whole machine (all logical processors = 100).

#### Scenario: Metrics after the first interval
- **WHEN** an authorized client sends `GET /api/v1/cpu/metrics` at least one interval after startup
- **THEN** the response status is 200
- **AND** `current.usagePercent` is between 0 and 100
- **AND** `current.perCoreUsagePercent` has one element per logical processor
- **AND** `current.topProcesses` has at most 10 elements sorted by `cpuPercent`, highest first

#### Scenario: Metrics before the first interval
- **WHEN** a client sends `GET /api/v1/cpu/metrics` before the first interval has been measured
- **THEN** the response status is 503 with a problem body

#### Scenario: Counter wrap-around
- **WHEN** a cumulative 32-bit event counter wraps between two samples
- **THEN** the rate of that interval is computed across the wrap instead of becoming negative

### Requirement: Platform without a CPU provider answers 501
When the platform library provides no CPU provider, `GET /api/v1/cpu/info` and
`GET /api/v1/cpu/metrics` SHALL respond with status 501 and a problem body, and the backend SHALL keep
serving other endpoints.

#### Scenario: CPU provider unavailable
- **WHEN** the backend started without a CPU provider (for example on macOS) and a client sends `GET /api/v1/cpu/metrics`
- **THEN** the response status is 501
