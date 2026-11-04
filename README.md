# SystemLens

A Windows task manager (inspired by MiTeC Task Manager DeLuxe) built as a learning project.

- **Backend:** C++20, CMake — collects system data, exposes it over a local HTTP/WebSocket API.
- **Frontend:** Electron (HTML/CSS/JS) — displays the data.

## Layout

```
docs/        Document
api/         API contract: openapi.yaml (REST). Source of truth for BE & FE.
backend/     C++20 + CMake (deps via FetchContent: cpp-httplib, nlohmann/json)
frontend/    UI
Tool/        PowerShell helper scripts (work from any current directory)
.claude/     project-only Claude Code settings + hooks (fix-format on Write/Edit)
```

## Commands

Quick run (PowerShell, from anywhere):

```powershell
Tool\run-backend.ps1        # configure + build Debug + run backend; no args = --dev --port 8765 (no auth)
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
```

Debug in VS Code: open the repo root, pick the "Debug backend" launch config (F5). It builds first
and starts with `--dev --port 8765`.

Frontend:

```bash
cd frontend && npm install && npm start
```

Linux (GCC or Clang, Ninja): the frontend starts `backend/build/systemlens-backend`.

```bash
cmake -S backend -B backend/build -G Ninja -DCMAKE_BUILD_TYPE=Debug
cmake --build backend/build
cd frontend && npm install && npm start
```

Tests (kept apart from the production code):

```bash
# Backend: GoogleTest (backend/tests/)
cmake -S backend -B backend/build -DSYSTEMLENS_BUILD_TESTS=ON
cmake --build backend/build            # Windows: add --config Debug
ctest --test-dir backend/build --output-on-failure   # Windows: add -C Debug

# Frontend: Playwright end-to-end on the Electron app + real backend (build the backend first)
cd frontend
npm run test:e2e                       # Linux without a display: xvfb-run -a npm run test:e2e
npm run test:e2e:report                # HTML report with screenshots and videos
```

