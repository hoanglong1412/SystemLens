# API

| Item | Value |
|---|---|
| Contract | [`api/openapi.yaml`](../api/openapi.yaml): the single source of truth for backend and frontend ([RQ001-NFR-01](requirements/RQ001_CoreProject.md#rq001-nfr-01)). If this file and the YAML disagree, the YAML is right. |
| Architecture | How the pieces fit: [RQ001_Architecture.md](design/RQ001_CoreProject/RQ001_Architecture.md) |
| Contract version | `0.1.0` (OpenAPI `3.1.0`) |

| Section | Content |
|---|---|
| [1. Add an API](#1-add-an-api) | Steps and a template |
| [2. Rules](#2-rules) | URL, auth, CORS, responses, frontend and backend rules |
| [3. Endpoints](#3-endpoints) | What exists today |

## 1. Add an API

Spec first: steps 1–3 are reviewed before any code. The contract may be changed only with the owner's approval.
Example to copy: the sample endpoint `getSampleHello` (files in the "Sample" column).

| Step | Action | Where | Sample |
|---|---|---|---|
| 1 | Add the path, schemas and responses to the contract. Every endpoint has an `operationId` and a `tag`. | `api/openapi.yaml` | `getSampleHello` |
| 2 | Add a row to [§3](#3-endpoints). | this file | — |
| 3 | Add or update the requirement and the design. | `docs/requirements/`, `docs/design/` | — |
| 4 | Service interface and implementation. | `backend/src/services/<area>/I<Name>Service.h`, `<Name>Service.h/.cpp` | `ISampleHelloService`, `SampleHelloService` |
| 5 | Controller implementing `IController`. It registers its route in `registerRoutes()` and has no business logic. | `backend/src/api/<area>/<Name>Controller.h/.cpp` | `SampleHelloController` |
| 6 | Create the service and the controller in `Application::registerControllers()` (comment "Add new controllers here"). | `backend/src/app/Application.cpp` | — |
| 7 | Add the new `.cpp` files to `systemlens_core`. | `backend/CMakeLists.txt` | — |
| 8 | Frontend api module: one exported function per endpoint, taking an `ApiClient` and returning plain data; export the path as a constant. | `frontend/renderer/api/<area>/<name>Api.js` | `sampleHelloApi.js` |

Controller shape (from `SampleHelloController`):

```cpp
class XxxController final : public IController
{
public:
    explicit XxxController(std::shared_ptr<const IXxxService> service);   // throws if null
    void registerRoutes(IRouteRegistry& registry) override;
private:
    Response handleGetXxx(const Request& request) const;
};

void XxxController::registerRoutes(IRouteRegistry& registry)
{
    registry.addRoute(HttpMethod::Get, kRoute, [this](const Request& r) { return handleGetXxx(r); });
}
Response XxxController::handleGetXxx(const Request&) const { return ok({{"field", m_service->getXxx()}}); }
```

Do not touch `HttpServer`, `ApiClient` or the preload to add an endpoint: auth, CORS, errors, the token and the
timeout are already handled for every route.

## 2. Rules

### Transport and URL

| Rule | Source |
|---|---|
| HTTP on `127.0.0.1` only. The port is chosen by the OS (`--port 0`) and reported by the READY line (`8765` when started by `Tool/run-backend.ps1`). | `HttpServerOptions::host`, `Application::announceReady` |
| Path `/api/v1/<area>/<resource>`; routing is an exact method + path match. | `api/openapi.yaml`, `IRouteRegistry::addRoute` |
| Methods the route registry supports: `GET`, `POST`, `PUT`, `DELETE`. | `HttpMethod` in `HttpTypes.h` |

### Authentication and CORS

| Rule | Source |
|---|---|
| Every request carries `Authorization: Bearer <token>`. The token is created per run by Electron main and given to the backend in `SYSTEMLENS_TOKEN`. | `ApiClient`, `BackendProcess.start` |
| The backend checks the token before routing, in constant time. Controllers never handle auth. Missing or wrong token → 401. | `HttpServer::installMiddleware` |
| `--dev` disables authentication (local development and curl only). | `AppConfig.h`, `Application.cpp` |
| Every response has `Access-Control-Allow-Origin: *`, `-Allow-Headers: Authorization, Content-Type`, `-Allow-Methods: GET, POST, PUT, DELETE, OPTIONS`. `OPTIONS` is answered 204 before the auth check. | `addCorsHeaders`, `HttpServer.cpp` |
| The renderer CSP allows `connect-src http://127.0.0.1:*`. | `frontend/renderer/index.html` |

### Responses

| Case | Status | Content type | Body | Built by |
|---|---|---|---|---|
| Success | `200` by default | `application/json` | JSON of the resource | `api::ok(body, statusCode)` |
| Error | 4xx / 5xx | `application/problem+json` | RFC 7807 `Problem`: `type` = `"about:blank"`, `title`, `status`, `detail` | `api::problem(statusCode, title, detail)` |

`ApiResponse.h` is the only place that builds responses. Status codes come from `api::status` (no magic
numbers): `kOk` 200, `kNoContent` 204, `kBadRequest` 400, `kUnauthorized` 401, `kNotFound` 404,
`kInternalServerError` 500.

Errors the server produces itself:

| Situation | Status | `title` | `detail` |
|---|---|---|---|
| Missing or invalid token | 401 | `Unauthorized` | `Missing or invalid bearer token.` |
| No route for method + path | 404 | `Not Found` | `No route for <METHOD> <path>` |
| Exception thrown by a handler | 500 | `Internal Server Error` | the exception's `what()`, or `Unknown error` |

### Frontend and backend

| Rule | Source |
|---|---|
| Every call goes through `ApiClient`, never `fetch` directly. Any failure (HTTP error, network failure, timeout of 5000 ms) becomes an `ApiError` with `status` (0 when no response) and `title`. | `apiClient.js` |
| Layering: `HttpServer` → Controller (`IController`) → Service (interface + implementation). A controller has no business logic. | `IController.h` |
| Services reach controllers through the constructor as interfaces; a null service throws `std::invalid_argument`. | `SampleHelloController.cpp` |
| Logs go to stderr; stdout is reserved for the `SYSTEMLENS_READY` line. | `Application.h`, `HttpServer.cpp` |

## 3. Endpoints

Security: every endpoint uses `bearerAuth` (see Authentication). Tag `sample`: sample endpoints that exercise
the FE ↔ BE plumbing and serve as reference examples; registered only when the backend is built with
`SYSTEMLENS_ENABLE_SAMPLE_API=ON`.

| Method | Path | `operationId` | Responses | Requirement |
|---|---|---|---|---|
| `GET` | `/api/v1/sample/hello` | `getSampleHello` | 200 `SampleHelloResponse`, 401, 500 | [RQ001-FR-01](requirements/RQ001_CoreProject.md#rq001-fr-01) |

Schemas:

| Schema | Fields |
|---|---|
| `SampleHelloResponse` | `message` (string, required): always `"@@Hello world@@"`. Example: `{"message": "@@Hello world@@"}` |
| `Problem` | `type` (string, required), `title` (string, required), `status` (integer, required), `detail` (string, optional). Media type `application/problem+json`. |

The contract also defines reusable responses `Unauthorized` (401), `NotFound` (404) and `InternalError` (500),
all with the `Problem` schema. `NotFound` is not referenced by any endpoint; the backend returns it for any
unknown path.
