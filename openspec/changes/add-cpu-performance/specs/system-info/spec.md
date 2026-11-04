# Spec Delta

## Purpose

Serves the identity of the machine the backend runs on, shown in the header of the app shell.

## ADDED Requirements

### Requirement: Machine identity
`GET /api/v1/system/info` SHALL respond with status 200 and a `SystemInfo` body: `hostName`, `osName`,
`osVersion`, `userName` and `platform` (`"windows"`, `"linux"` or `"macos"`). A text the platform cannot
report SHALL be an empty string.

#### Scenario: Info on Linux
- **WHEN** an authorized client sends `GET /api/v1/system/info` to a backend running on Linux
- **THEN** the response status is 200
- **AND** `platform` is `"linux"` and `hostName` is the kernel host name

### Requirement: Platform without a system info provider answers 501
When the platform library provides no system info provider, `GET /api/v1/system/info` SHALL respond
with status 501 and a problem body.

#### Scenario: System info provider unavailable
- **WHEN** the backend started without a system info provider and a client sends `GET /api/v1/system/info`
- **THEN** the response status is 501
