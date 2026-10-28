# API convention

| Item | Value |
|---|---|
| Scope | Rules every backend API follows, as applied by the current code |
| Traces to | [RQ001-NFR-01](requirements/RQ001_CoreProject.md#rq001-nfr-01), [RQ001-NFR-02](requirements/RQ001_CoreProject.md#rq001-nfr-02) |
| Contract | [`api/openapi.yaml`](../api/openapi.yaml) (source of truth), tables in [api-schema.md](api-schema.md) |
| Detailed in | [RQ001-HLD-03](design/RQ001_CoreProject/RQ001_HLD.md#rq001-hld-03), [RQ001-LLD-02](design/RQ001_CoreProject/RQ001_LLD-02-api-request-pipeline.md) |

Referenced from `api/openapi.yaml`, `backend/src/api/ApiResponse.h` and `frontend/renderer/api/apiClient.js`.

## 1. Contract

| Rule | Source |
|---|---|
| `api/openapi.yaml` is the source of truth for backend and frontend. Change it only with the owner's approval. | `api/openapi.yaml` header |
| Every endpoint has an `operationId` and a `tag`. | `api/openapi.yaml` |
| Controllers and FE api modules name the contract in their file header. | `SampleHelloController.h`, `sampleHelloApi.js` |

## 2. Transport and URL

| Rule | Value | Source |
|---|---|---|
| Protocol | HTTP | `HttpServer` (cpp-httplib) |
| Host | `127.0.0.1` only (loopback) | `HttpServerOptions::host`, `api/openapi.yaml` `servers` |
| Port | Chosen by the OS (`--port 0`) and reported with `SYSTEMLENS_READY {"port":<n>}` | `Application::announceReady`, `BackendProcess.start` |
| Path | `/api/v1/<area>/<resource>` — e.g. `/api/v1/sample/hello` | `api/openapi.yaml` |
| Routing | Exact method + path match | `IRouteRegistry::addRoute` |
| Methods supported by the route registry | `GET`, `POST`, `PUT`, `DELETE` | `HttpMethod` in `HttpTypes.h` |

## 3. Authentication

| Rule | Source |
|---|---|
| Every request carries `Authorization: Bearer <token>`. | `ApiClient.#request`, `api/openapi.yaml` `security` |
| The token is generated per run by Electron main (32 random bytes, hex) and given to the backend in the `SYSTEMLENS_TOKEN` environment variable, not on the command line. | `BackendProcess.start`, `AppConfig.h` |
| The backend checks the token before routing, with a constant-time comparison. Controllers never handle auth. | `HttpServer::installMiddleware` |
| Missing / wrong token → `401` `Problem`. | `HttpServer::installMiddleware` |
| `--dev` disables authentication (local development / curl only). | `AppConfig.h`, `Application.cpp` (`toServerOptions`) |

## 4. CORS

| Rule | Source |
|---|---|
| Every response has `Access-Control-Allow-Origin: *`, `Access-Control-Allow-Headers: Authorization, Content-Type`, `Access-Control-Allow-Methods: GET, POST, PUT, DELETE, OPTIONS` — the renderer is loaded from `file://` (origin `null`). | `addCorsHeaders` in `HttpServer.cpp` |
| `OPTIONS` (preflight) is answered `204` before the auth check. | `HttpServer::installMiddleware` |
| The renderer CSP allows `connect-src http://127.0.0.1:*`. | `frontend/renderer/index.html` |

## 5. Responses

| Case | Status | Content type | Body | Built by |
|---|---|---|---|---|
| Success | `200` by default | `application/json` | JSON of the resource | `api::ok(body, statusCode)` |
| Error | 4xx / 5xx | `application/problem+json` | RFC 7807 `Problem`: `type` = `"about:blank"`, `title`, `status`, `detail` | `api::problem(statusCode, title, detail)` |

`ApiResponse.h` is the only place where responses are built. Status codes come from the constants in
`api::status` (no magic numbers):

| Constant | Value |
|---|---|
| `kOk` | 200 |
| `kNoContent` | 204 |
| `kBadRequest` | 400 |
| `kUnauthorized` | 401 |
| `kNotFound` | 404 |
| `kInternalServerError` | 500 |

Errors produced by the server itself (not by controllers):

| Situation | Status | `title` | `detail` |
|---|---|---|---|
| Missing / invalid token | 401 | `Unauthorized` | `Missing or invalid bearer token.` |
| No route for method + path | 404 | `Not Found` | `No route for <METHOD> <path>` |
| Exception thrown by a handler | 500 | `Internal Server Error` | exception `what()` (or `Unknown error`) |

## 6. Frontend side

| Rule | Source |
|---|---|
| Every API call goes through `ApiClient`, never `fetch` directly. | `apiClient.js` header |
| One api module per area under `frontend/renderer/api/<area>/`, one exported function per endpoint, taking an `ApiClient` and returning plain data. | `sampleHelloApi.js` header |
| The path is exported as a constant from the api module (e.g. `SAMPLE_HELLO_PATH`). | `sampleHelloApi.js` |
| Any failure (HTTP error, network failure, timeout of 5000 ms) becomes an `ApiError` with `status` (0 when no response) and `title`. | `ApiClient` |

## 7. Backend side

| Rule | Source |
|---|---|
| Layering: `HttpServer` → Controller (`IController`) → Service (interface + implementation). | `IController.h`, `ISampleHelloService.h` |
| A controller owns no business logic: Request → service call → `ok(...)`. | `IController.h`, `SampleHelloController.h` |
| Services are injected into controllers through the constructor as interfaces; a null service throws `std::invalid_argument`. | `SampleHelloController.cpp` |
| Controllers are created and registered in `Application::registerControllers()` (composition root). | `Application.cpp` |
| Logs go to stderr; stdout is reserved for the `SYSTEMLENS_READY` line. | `Application.h`, `HttpServer.cpp` |

## 8. Adding an API

Order taken from the current code structure and the project rule "spec first":

| Step | Action | Where |
|---|---|---|
| 1 | Add the endpoint, schemas and responses to the contract. | `api/openapi.yaml` |
| 2 | Update the readable tables. | [api-schema.md](api-schema.md) |
| 3 | Add / update the requirement and design docs. | `docs/requirements/`, `docs/design/` |
| 4 | Add the service interface and implementation. | `backend/src/services/<area>/I<Name>Service.h`, `<Name>Service.h/.cpp` |
| 5 | Add the controller implementing `IController`. | `backend/src/api/<area>/<Name>Controller.h/.cpp` |
| 6 | Create service + controller in `Application::registerControllers()` ("Add new controllers here"). | `backend/src/app/Application.cpp` |
| 7 | Add the `.cpp` files to `systemlens_core`. | `backend/CMakeLists.txt` |
| 8 | Add the FE api module function. | `frontend/renderer/api/<area>/<name>Api.js` |

Example: the sample endpoint `getSampleHello` ([RQ001-LLD-02 §6](design/RQ001_CoreProject/RQ001_LLD-02-api-request-pipeline.md#rq001-lld-02-sample)).
