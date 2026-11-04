# Spec Delta

## Purpose

Provides the sample endpoint `GET /api/v1/sample/hello`, which proves frontend-to-backend communication
and that the backend reaches its platform library, by returning a greeting that names the platform.

## ADDED Requirements

### Requirement: Greeting identifies the platform
`GET /api/v1/sample/hello` SHALL respond with status 200 and a JSON body whose `message` is the greeting
supplied by the platform library: `"hello from Windows"` on Windows, `"hello from Linux"` on Linux and
`"hello from MAC"` on macOS.

#### Scenario: Request on Windows
- **WHEN** an authorized client sends `GET /api/v1/sample/hello` to a backend running on Windows
- **THEN** the response status is 200
- **AND** the body is `{"message": "hello from Windows"}`

#### Scenario: Request on Linux
- **WHEN** an authorized client sends `GET /api/v1/sample/hello` to a backend running on Linux
- **THEN** the response status is 200
- **AND** the body is `{"message": "hello from Linux"}`

#### Scenario: Request on macOS
- **WHEN** an authorized client sends `GET /api/v1/sample/hello` to a backend running on macOS
- **THEN** the response status is 200
- **AND** the body is `{"message": "hello from MAC"}`

### Requirement: Platform without a hello provider answers 501
When the platform library provides no hello provider, `GET /api/v1/sample/hello` SHALL respond with
status 501 and a problem body (`application/problem+json`), and the backend SHALL keep serving other
endpoints.

#### Scenario: Hello provider unavailable
- **WHEN** the backend started without a hello provider and an authorized client sends `GET /api/v1/sample/hello`
- **THEN** the response status is 501
- **AND** the body is a problem with `status` 501
