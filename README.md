# SystemLens

A Windows task manager (inspired by MiTeC Task Manager DeLuxe) built as a learning project.

- **Backend:** C++20, CMake — collects system data, exposes it over a local HTTP/WebSocket API.
- **Frontend:** Electron (HTML/CSS/JS) — displays the data.

## Layout

```
docs/        SystemLens_spec.md (master spec: features, FR/NFR, acceptance, traceability),
             high-level-design.md, lld/backend-lld.md, lld/frontend-lld.md,
             api-reference.md (one table row per API, mirrors api/openapi.yaml — update together),
             api-convention.md, coding-convention.md, ADRs (docs/adr)
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

