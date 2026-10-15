# SystemLens — guide for Claude

Windows task manager clone. Learning project: the owner wants to understand OS internals, practice C++,
and practice spec-driven design with a human in the loop.

## Layout

```
docs/        vision, requirements, HLD, ADRs (docs/adr), low-level designs (docs/lld), glossary
api/         API contract: openapi.yaml (REST) + event schemas (stream). Source of truth for BE & FE.
backend/     C++20 + CMake. src/ = code, tests/ = API tests + interaction tests
frontend/    Electron. main/ = main process, preload/ = contextBridge, renderer/ = UI (vanilla JS)
Tool/        PowerShell helper scripts (work from any current directory)
```

## Commands

Quick run (PowerShell, from anywhere):

```powershell
Tool\run-backend.ps1    # configure + build Debug + run backend (extra args are passed to the exe)
Tool\run-frontend.ps1   # npm install on first run, then start Electron
```

Backend (MSVC from Visual Studio 2019, generator "Visual Studio 16 2019"). Run from the **repo root**:

```bash
cmake -S backend -B backend/build -G "Visual Studio 16 2019" -A x64
cmake --build backend/build --config Debug
backend/build/Debug/systemlens-backend.exe
```

Debug in VS Code: open the repo root, pick the "Debug backend" launch config (F5). It builds first.

Frontend:

```bash
cd frontend && npm install && npm start
```

## Rules

- **Contract first.** Never change `api/` without an approved ADR / explicit user approval. BE and FE
  are developed in parallel against the contract.
- **Human gates:** ADRs, API contract changes and merges to `main` need the user's approval.
- **Every commit updates `HISTORY.md`** (in the same commit): add an entry at the top with the reason,
  what the commit does, and the relevant user ↔ AI exchanges (requests, decisions, issues found).
  Follow the template at the top of that file. Write it in Vietnamese.
- **No new tooling/installs without asking first** (compilers, extensions, global packages, vcpkg ports).
- Code quality is reviewed manually: no linters/formatters in the pipeline.
- Frontend is tested manually. Backend has two test suites:
  - **API tests** — black-box HTTP against a running backend (Fake provider), validated against `api/`.
  - **Interaction tests** — gmock-based, verify layers call each other correctly (API → Service → Provider).
