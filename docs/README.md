# SystemLens documentation

Entry point of the project documentation. All documents describe the **current source code**. Requirements are
added by the project owner while the project goes on. Writing rules: [CLAUDE.md](CLAUDE.md).

## 1. Where to look

| I want to know | Read |
|---|---|
| What the project must do | [requirements/RQ001_CoreProject.md](requirements/RQ001_CoreProject.md), [requirements/RQ002_CpuPerformance.md](requirements/RQ002_CpuPerformance.md) |
| How many components there are, how they talk, how start / request / stop run, what happens on failure, why | [design/RQ001_CoreProject/RQ001_Architecture.md](design/RQ001_CoreProject/RQ001_Architecture.md) |
| How the CPU screen gets its data (sampling, computations, OS sources, tests) | [design/RQ002_CpuPerformance/RQ002_Architecture.md](design/RQ002_CpuPerformance/RQ002_Architecture.md) |
| How to add an API, the API rules, the endpoint list | [api-guide.md](api/api-guide.md) |
| The exact contract | [`api/openapi.yaml`](../api/openapi.yaml) |

```
docs/
├── README.md                          this file
├── CLAUDE.md                          rules for writing documentation
├── api/
│   └── api-guide.md                   add an API, API rules, endpoint list
├── requirements/                      one file per work item (RQ<NNN>_<Name>.md)
│   ├── RQ001_CoreProject.md           FR, UC, NFR of the project core
│   └── RQ002_CpuPerformance.md        FR, UC, NFR of the Performance › CPU screen
├── design/                            one folder per requirement file (RQ<NNN>_<Name>/)
│   ├── RQ001_CoreProject/
│   │   └── RQ001_Architecture.md      components, flows, failures, decisions, requirement coverage
│   └── RQ002_CpuPerformance/
│       └── RQ002_Architecture.md      CPU data path, computations, OS sources, failures, tests
└── ui-design/
    └── figma.onl                      link to the Figma file of the UI
```

## 2. Requirement files

| RQ | Requirement | Design | Title | Status |
|---|---|---|---|---|
| RQ001 | [RQ001_CoreProject.md](requirements/RQ001_CoreProject.md) | [RQ001_Architecture.md](design/RQ001_CoreProject/RQ001_Architecture.md) | Core project | Partially implemented |
| RQ002 | [RQ002_CpuPerformance.md](requirements/RQ002_CpuPerformance.md) | [RQ002_Architecture.md](design/RQ002_CpuPerformance/RQ002_Architecture.md) | CPU performance screen | Implemented |

IDs: `RQ<NNN>-FR-NN`, `RQ<NNN>-UC-NN`, `RQ<NNN>-NFR-NN` are defined in the requirement file and never reused or
renumbered; a removed item keeps its ID with status `Removed`. An API endpoint is identified by its
`operationId`. Design decisions are `D1`, `D2`, … in the architecture file.

## 3. Requirement → design → code

| Requirement | Design | API | Source |
|---|---|---|---|
| RQ001-FR-01 / UC-01 | Architecture §3.1, §3.2, §4 | `getSampleHello` | `frontend/renderer/app.js`, `frontend/renderer/api/sample/*`, `backend/src/api/sample/*`, `backend/src/services/sample/*` |
| RQ001-NFR-01 | Architecture §6 D1, D9; [api-guide.md](api/api-guide.md) | all | `api/openapi.yaml` |
| RQ001-NFR-02 | Architecture §7 (code that knows the IPC method) | all | `frontend/renderer/api/apiClient.js`, `frontend/main/*`, `frontend/preload/preload.js`, `backend/src/api/HttpServer.*`, `backend/src/app/*` |
| RQ001-NFR-03 | Architecture §7: not implemented | — | — |
| RQ002-FR-01, FR-02 | RQ002 Architecture §1–§4 | `getCpuInfo`, `getCpuMetrics` | `backend/src/{providers,services,api}/cpu/*`, `backend/plugins/{windows,linux}/*Cpu*`, `frontend/renderer/views/cpu/*`, `frontend/renderer/api/cpu/*` |
| RQ002-FR-03 | RQ002 Architecture §1, §5 | `getSystemInfo` | `backend/src/{providers,services,api}/system/*`, `backend/plugins/{windows,linux}/*SystemInfo*`, `frontend/renderer/shell/*` |
| RQ002-NFR-01 | RQ002 Architecture §5, §6 | `getCpuInfo`, `getCpuMetrics`, `getSystemInfo` | `backend/plugins/*` |
| RQ002-NFR-02 | RQ002 Architecture §7 | — | `backend/tests/*`, `frontend/tests/e2e/*`, `frontend/playwright.config.js` |

## 4. Updating the docs

Spec first: documents are updated before code.

1. Requirement: create `requirements/RQ<NNN>_<Name>.md` (next free number) or update the existing one. The
   owner provides the content. Add it to §2.
2. API: change `api/openapi.yaml`, then [api-guide.md](api/api-guide.md) ([steps](api/api-guide.md#1-add-an-api)).
3. Design: update the architecture file of that requirement. Create `design/RQ<NNN>_<Name>/RQ<NNN>_<Topic>.md`
   only when one file would pass about 400 lines or the work item has its own architecture.
4. Update §3 when requirements move.
