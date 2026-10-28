# SystemLens documentation

Entry point of the project documentation: what each file contains, how IDs work and how to trace a
requirement down to the design, the API and the source code.

All documents describe the **current source code**. Requirements are added by the project owner while the
project goes on; sections with no information yet only hold a template and an example.

## 1. Layout

```
docs/
├── README.md                                    this file: index, ID scheme, requirement files, traceability matrix
├── CLAUDE.md                                    rules for writing documentation
├── api-schema.md                                API contract as tables (generated from api/openapi.yaml)
├── api-convention.md                            rules every API follows (URL, auth, errors, how to add an API)
├── requirements/                                one file per work item (RQ<NNN>_<Name>.md)
│   └── RQ001_CoreProject.md                     FR, UC, NFR of the project core
├── design/                                      one folder per requirement file (RQ<NNN>_<Name>/)
│   └── RQ001_CoreProject/
│       ├── RQ001_HLD.md                         components, design decisions, how each requirement is met
│       ├── RQ001_LLD-01-backend-lifecycle.md    start / handshake / shutdown of the backend process
│       └── RQ001_LLD-02-api-request-pipeline.md one API call end to end, errors, sample greeting feature
└── ui-design/                                   Figma / UI design files (none yet)
```

Source of truth outside `docs/`:

```
api/
└── openapi.yaml                                 API contract, source of truth for backend and frontend
```

## 2. ID scheme

| Prefix | Meaning | Defined in | Example |
|---|---|---|---|
| `RQ<NNN>` | Requirement file = one work item | `requirements/RQ<NNN>_<Name>.md` | `RQ001` |
| `RQ<NNN>-FR-NN` | Functional requirement of that work item | the RQ file, section 2 | `RQ001-FR-01` |
| `RQ<NNN>-UC-NN` | Use case realizing an FR | the RQ file, under its FR | `RQ001-UC-01` |
| `RQ<NNN>-NFR-NN` | Non-functional requirement of that work item | the RQ file, section 3 | `RQ001-NFR-01` |
| `RQ<NNN>-HLD-NN` | Section of the high-level design of that work item | `design/RQ<NNN>_<Name>/RQ<NNN>_HLD.md` | `RQ001-HLD-03` |
| `RQ<NNN>-LLD-NN` | Low-level design of one feature of that work item | `design/RQ<NNN>_<Name>/RQ<NNN>_LLD-NN-*.md` | `RQ001-LLD-01` |
| `<ID> §<section> #<n>` | Step `n` of the numbered sequence diagram in that section | same file, step table | `RQ001-LLD-01 §3.1 #5` |
| `operationId` | One API endpoint | [api-schema.md](api-schema.md), `api/openapi.yaml` | `getSampleHello` |

Rules:
- An ID is never reused or renumbered; a removed item keeps its ID with status `Removed`.
- Anchors are the lowercase IDs (e.g. `#rq001-fr-01`, `#rq001-hld-02`); LLD sections use `#rq<nnn>-lld-nn-<topic>`.
- Each requirement file `requirements/RQ<NNN>_<Name>.md` has a design folder `design/RQ<NNN>_<Name>/`; design file
  names start with `RQ<NNN>_`. A folder holds as many HLD / LLD files as the scope of the requirement needs.
- Writing rules (diagrams, step tables, what a design contains): [CLAUDE.md](CLAUDE.md).

## 3. Requirement files

| RQ | Requirement file | Design folder | Title | Status | FR | NFR |
|---|---|---|---|---|---|---|
| RQ001 | [RQ001_CoreProject.md](requirements/RQ001_CoreProject.md) | [design/RQ001_CoreProject/](design/RQ001_CoreProject/) | Core project | Partially implemented | 1 | 3 |

## 4. Traceability matrix

| Requirement | HLD | LLD | API (`operationId`) | Source |
|---|---|---|---|---|
| [RQ001-FR-01](requirements/RQ001_CoreProject.md#rq001-fr-01) / [RQ001-UC-01](requirements/RQ001_CoreProject.md#rq001-uc-01) | [RQ001-HLD-04](design/RQ001_CoreProject/RQ001_HLD.md#rq001-hld-04) | [RQ001-LLD-02 §6](design/RQ001_CoreProject/RQ001_LLD-02-api-request-pipeline.md#rq001-lld-02-sample), [RQ001-LLD-01 §5](design/RQ001_CoreProject/RQ001_LLD-01-backend-lifecycle.md#rq001-lld-01-failures) | [`getSampleHello`](api-schema.md#getsamplehello) | `frontend/renderer/app.js`, `frontend/renderer/api/sample/sampleHelloApi.js`, `backend/src/api/sample/*`, `backend/src/services/sample/*` |
| [RQ001-NFR-01](requirements/RQ001_CoreProject.md#rq001-nfr-01) | [RQ001-HLD-02](design/RQ001_CoreProject/RQ001_HLD.md#rq001-hld-02) D1, D9, [RQ001-HLD-03](design/RQ001_CoreProject/RQ001_HLD.md#rq001-hld-03) | [RQ001-LLD-02](design/RQ001_CoreProject/RQ001_LLD-02-api-request-pipeline.md) | all | `api/openapi.yaml` |
| [RQ001-NFR-02](requirements/RQ001_CoreProject.md#rq001-nfr-02) | [RQ001-HLD-01](design/RQ001_CoreProject/RQ001_HLD.md#rq001-hld-01), [RQ001-HLD-02](design/RQ001_CoreProject/RQ001_HLD.md#rq001-hld-02) D2–D8, D10, [RQ001-HLD-03](design/RQ001_CoreProject/RQ001_HLD.md#rq001-hld-03-nfr-02) | [RQ001-LLD-01](design/RQ001_CoreProject/RQ001_LLD-01-backend-lifecycle.md), [RQ001-LLD-02](design/RQ001_CoreProject/RQ001_LLD-02-api-request-pipeline.md) | all | `frontend/renderer/api/apiClient.js`, `frontend/main/ipcChannels.js`, `frontend/preload/preload.js`, `backend/src/api/HttpTypes.h`, `backend/src/api/IRouteRegistry.h`, `backend/src/api/HttpServer.*` |
| [RQ001-NFR-03](requirements/RQ001_CoreProject.md#rq001-nfr-03) | [RQ001-HLD-03](design/RQ001_CoreProject/RQ001_HLD.md#rq001-hld-03-nfr-03): no design yet (not implemented) | — | — | — |

## 5. Updating the docs

Per the project rule "spec first", documents are updated before code. There are no templates: follow RQ001
as the example.

1. Requirement: for a new work item, create `requirements/RQ<NNN>_<Name>.md` (next free number) and add it to
   section 3; otherwise update the existing RQ file. The owner provides the content.
2. API: change `api/openapi.yaml`, then [api-schema.md](api-schema.md).
3. Design: create or update `design/RQ<NNN>_<Name>/` with the HLD / LLD files the scope needs
   (`RQ<NNN>_HLD.md`, `RQ<NNN>_LLD-NN-<name>.md`).
4. Add the new rows/links to the traceability matrix in section 4.
