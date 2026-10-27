# SystemLens documentation

Entry point of the project documentation: what each file contains, how IDs work and how to trace a
requirement down to the design, the API and the source code.

All documents describe the **current source code**. Requirements are added by the project owner while the
project goes on; sections with no information yet only hold a template and an example.

## 1. Layout

```
docs/
├── README.md                              this file: index, ID scheme, traceability matrix
├── api-schema.md                          API contract as tables (generated from api/openapi.yaml)
├── api-convention.md                      rules every API follows (URL, auth, errors, how to add an API)
├── requirements/
│   ├── functional-requirements.md         FR-* and UC-*: use cases and UI operations
│   └── non-functional-requirements.md     NFR-*: dependency reduction / extensibility rules
├── design/
│   ├── high-level-design.md               HLD-*: component diagram and overview flows
│   └── lld/
│       ├── _template.md                   template for a new LLD
│       ├── LLD-01-backend-lifecycle.md    start / handshake / shutdown of the backend process
│       ├── LLD-02-api-request-pipeline.md one API call from ApiClient to the controller and back
│       └── LLD-03-sample-hello.md         sample feature: show the greeting from GET /api/v1/sample/hello
└── ui-design/                             Figma / UI design files (none yet)
```

Source of truth outside `docs/`:

```
api/
└── openapi.yaml                           API contract, source of truth for backend and frontend
```

## 2. ID scheme

| Prefix | Meaning | Defined in | Example |
|---|---|---|---|
| `FR-<AREA>-NN` | Functional requirement | [functional-requirements.md](requirements/functional-requirements.md) | `FR-SMP-01` |
| `UC-<AREA>-NN` | Use case realizing an FR | [functional-requirements.md](requirements/functional-requirements.md) | `UC-SMP-01` |
| `NFR-NN` | Non-functional requirement | [non-functional-requirements.md](requirements/non-functional-requirements.md) | `NFR-01` |
| `HLD-NN` | Section of the high-level design | [high-level-design.md](design/high-level-design.md) | `HLD-03` |
| `LLD-NN` | Low-level design of one feature | `design/lld/LLD-NN-*.md` | `LLD-01` |
| `HLD-NN#n` / `LLD-NN#n` | Step `n` of the numbered sequence diagram in that section | same file, step table | `HLD-03#4` |
| `operationId` | One API endpoint | [api-schema.md](api-schema.md), `api/openapi.yaml` | `getSampleHello` |

Areas currently in use:

| Area | Meaning |
|---|---|
| `SMP` | Sample API / sample feature (exists only to prove FE ↔ BE communication) |

Rules:
- An ID is never reused or renumbered; a removed item keeps its ID with status `Removed`.
- Every document starts with a metadata table: **Traces to** (upstream items) and **Detailed in** (downstream items).
- Every numbered sequence diagram (`autonumber`) is followed by a step table whose `#` column matches the numbers.

## 3. Traceability matrix

| Requirement | HLD | LLD | API (`operationId`) | Source |
|---|---|---|---|---|
| [FR-SMP-01](requirements/functional-requirements.md#fr-smp-01) / [UC-SMP-01](requirements/functional-requirements.md#uc-smp-01) | [HLD-02](design/high-level-design.md#hld-02) | [LLD-03](design/lld/LLD-03-sample-hello.md) | [`getSampleHello`](api-schema.md#getsamplehello) | `frontend/renderer/app.js`, `frontend/renderer/api/sample/sampleHelloApi.js`, `backend/src/api/sample/*`, `backend/src/services/sample/*` |
| [NFR-01](requirements/non-functional-requirements.md#nfr-01) | [HLD-01](design/high-level-design.md#hld-01), [HLD-05](design/high-level-design.md#hld-05) | [LLD-02](design/lld/LLD-02-api-request-pipeline.md) | all | `api/openapi.yaml` |
| [NFR-02](requirements/non-functional-requirements.md#nfr-02) | [HLD-01](design/high-level-design.md#hld-01), [HLD-03](design/high-level-design.md#hld-03), [HLD-05](design/high-level-design.md#hld-05) | [LLD-01](design/lld/LLD-01-backend-lifecycle.md), [LLD-02](design/lld/LLD-02-api-request-pipeline.md) | all | `frontend/renderer/api/apiClient.js`, `frontend/main/ipcChannels.js`, `frontend/preload/preload.js`, `backend/src/api/HttpTypes.h`, `backend/src/api/IRouteRegistry.h`, `backend/src/api/HttpServer.*` |
| [NFR-03](requirements/non-functional-requirements.md#nfr-03) | — (not implemented) | — | — | — |

Infrastructure flows that every feature relies on (no FR of their own):

| Flow | HLD | LLD |
|---|---|---|
| App start / end, backend handshake | [HLD-03](design/high-level-design.md#hld-03), [HLD-04](design/high-level-design.md#hld-04) | [LLD-01](design/lld/LLD-01-backend-lifecycle.md) |
| Calling one API | [HLD-05](design/high-level-design.md#hld-05) | [LLD-02](design/lld/LLD-02-api-request-pipeline.md) |

## 4. Updating the docs

Per the project rule "spec first", documents are updated before code:

1. Requirement: add/update the FR/UC or NFR (owner provides the content).
2. API: change `api/openapi.yaml`, then [api-schema.md](api-schema.md).
3. Design: update [high-level-design.md](design/high-level-design.md) if the overview changes; add or update the LLD
   (copy [_template.md](design/lld/_template.md)).
4. Add the new row/links to the traceability matrix above.
