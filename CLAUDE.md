# SystemLens — guide for Claude

Windows task manager clone. Learning project: the owner wants to understand OS internals, practice C++,
and practice spec-driven design with a human in the loop.

## Layout

```
docs/        high-level-design.md, api-convention.md, coding-convention.md, ADRs (docs/adr), LLD (docs/lld)
api/         API contract: openapi.yaml (REST). Source of truth for BE & FE.
backend/     C++20 + CMake (deps via FetchContent: cpp-httplib, nlohmann/json, googletest)
  src/app/       AppConfig (args/env), Application (composition root, READY handshake, stdin watchdog)
  src/api/       HttpServer (auth/CORS/errors), ApiResponse (ok/problem), IController, controllers per area
  src/services/  business logic behind I<Name>Service interfaces
  tests/         api/ (HTTP against a real HttpServer), interaction/ (gmock), support/ (fakes)
frontend/    Electron. main/ = main process + BackendProcess (spawns backend), preload/ = window.systemlens,
             renderer/ = UI; renderer/api/ = ApiClient + one module per API area
Tool/        PowerShell helper scripts (work from any current directory)
  githooks/  git hooks (pre-commit format check), enabled by Tool/setup-hooks.ps1
  lib/       shared helpers for the check tools
.claude/     project-only Claude Code settings + hooks (fix-format on Write/Edit)
```

## Commands

Quick run (PowerShell, from anywhere):

```powershell
Tool\run-backend.ps1        # configure + build Debug + run backend; no args = --dev --port 8765 (no auth)
Tool\run-backend-tests.ps1  # configure + build Debug + ctest (API tests + interaction tests)
Tool\run-frontend.ps1       # npm install on first run, then start Electron (it spawns the Debug backend)
Tool\check_utf8.ps1     # check UTF-8 without BOM   (-Path <files>, -Staged, -Fix)
Tool\check_crlf.ps1     # check CRLF line endings   (-Path <files>, -Staged, -Fix)
Tool\setup-hooks.ps1    # once per clone: enable Tool/githooks for this repo only
```

Backend (MSVC from Visual Studio 2019, generator "Visual Studio 16 2019"). Run from the **repo root**:

```bash
cmake -S backend -B backend/build -G "Visual Studio 16 2019" -A x64
cmake --build backend/build --config Debug
backend/build/Debug/systemlens-backend.exe --dev --port 8765
ctest --test-dir backend/build -C Debug --output-on-failure
```

Debug in VS Code: open the repo root, pick the "Debug backend" launch config (F5). It builds first
and starts with `--dev --port 8765`.

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
- **File format: every text file is UTF-8 without BOM + CRLF**, in the working tree and in the repo
  (`.gitattributes` has `* -text`, so git stores bytes as-is).
  - Write/Edit: the project hook `.claude/hooks/fix-format.ps1` converts the file automatically.
  - Any other way of changing files (Bash, sed, scripts, `npm install`…): run
    `Tool\check_utf8.ps1 -Path <file> -Fix` and `Tool\check_crlf.ps1 -Path <file> -Fix`.
  - Before reporting results to the user, run both tools (no `-Fix`) and make sure they exit 0.
  - The git pre-commit hook shows a Yes/No dialog listing invalid staged files; it never auto-fixes.
- **API work follows [docs/api-convention.md](docs/api-convention.md)** (URLs, JSON, problem+json errors, auth,
  `/api/v1/sample/*` + `Sample*`/`sample*` naming for sample (reference/plumbing) APIs, checklist for a new API).
  Architecture: [docs/high-level-design.md](docs/high-level-design.md) (diagrams in Mermaid).
- **No new tooling/installs without asking first** (compilers, extensions, global packages, vcpkg ports).
- Code quality is reviewed manually: no linters/formatters in the pipeline.
- Frontend is tested manually. Backend has two test suites:
  - **API tests** — black-box HTTP against a running backend (Fake provider), validated against `api/`.
  - **Interaction tests** — gmock-based, verify layers call each other correctly (API → Service → Provider).

## Coding convention

Full details and header templates: [docs/coding-convention.md](docs/coding-convention.md). Comments are in English.

1. **File header comment** on every file: file name + what the file contains / is responsible for.
2. **Function header comment** on every function: what it does, params, return value, errors thrown
   (C++: Doxygen at the declaration in the `.h`; JS: JSDoc).
3. **Comment non-obvious code**: explain *why* (reason, constraint, pitfall), not *what*.
4. **Clean, maintainable, extensible code**: single responsibility, depend on interfaces injected via
   constructor, respect the layering (HttpServer → Controller → Service → Provider), no magic numbers,
   explicit error handling, extend by adding files rather than modifying infrastructure.
