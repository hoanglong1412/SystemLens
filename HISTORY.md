# HISTORY

Nhật ký các commit: lý do, nội dung, và trao đổi giữa user với AI dẫn tới commit đó.
Entry mới nhất ở trên cùng. Hash commit xem bằng `git log` (entry được viết trong chính commit nên không ghi hash).

<!--
Mẫu entry:

## YYYY-MM-DD — <tiêu đề commit>

**Lý do:** vì sao cần commit này.

**Nội dung:**
- thay đổi 1
- thay đổi 2

**Trao đổi user ↔ AI:**
- User: yêu cầu / quyết định
- AI: đề xuất / kết quả / vấn đề phát hiện
-->

## 2026-10-16 — Thêm OpenSpec (commands, skills, config) với định dạng UTF-8 + CRLF

**Lý do:** User cài OpenSpec vào dự án; các file sinh ra đang dùng LF nên vi phạm rule định dạng của repo.

**Nội dung:**
- Thêm `.claude/commands/opsx/*.md` (apply, archive, explore, propose, sync, update),
  `.claude/skills/openspec-*/SKILL.md` (6 skill) và `openspec/config.yaml` (schema `spec-driven`).
- Chuyển cả 13 file từ LF sang CRLF (đã là UTF-8 không BOM) bằng `Tool\check_crlf.ps1 -Fix`; không sửa nội dung.

**Trao đổi user ↔ AI:**
- AI phát hiện 13 file OpenSpec (chưa track) sai định dạng CRLF khi kiểm tra toàn repo và hỏi cách xử lý.
- User yêu cầu chỉnh đúng định dạng và commit riêng phần OpenSpec trước, các thay đổi còn lại commit sau.
- Ghi chú: dự án dùng spec Markdown (`docs/SystemLens_spec.md`), chưa dùng quy trình change của OpenSpec.

## 2026-10-15 — Core giao tiếp FE ↔ BE + sample API "hello client" + coding convention

**Lý do:** Dựng khung giao tiếp Frontend ↔ Backend mà mọi API sau này sẽ tuân theo, chứng minh end-to-end
bằng một sample API, và thống nhất quy chuẩn code / API / tài liệu thiết kế.

**Nội dung:**
- Contract `api/openapi.yaml`: `GET /api/v1/sample/hello` (tag `sample`), bearer auth, schema `Problem` (RFC 7807).
- Backend (C++20, deps qua CMake FetchContent: cpp-httplib 0.18.3, nlohmann/json 3.12.0, googletest 1.15.2):
  - `HttpServer`: bind 127.0.0.1, auth bearer token, CORS cho renderer `file://`, 404/500 → problem+json.
  - Layer: `IController` / `IRouteRegistry` → `SampleHelloController` → `ISampleHelloService` / `SampleHelloService`.
  - `AppConfig` (`--port`, `--dev`, env `SYSTEMLENS_TOKEN`), `Application` (composition root, dòng handshake
    `SYSTEMLENS_READY {"port":n}`, tự thoát khi stdin đóng).
  - Option CMake `SYSTEMLENS_ENABLE_SAMPLE_API`, `SYSTEMLENS_BUILD_TESTS`.
  - Tests: 6 API test (200, 401 thiếu/sai token, CORS preflight, 404, 500) + 4 interaction test (gmock).
- Frontend: `BackendProcess` (spawn backend, token ngẫu nhiên, chờ READY, dừng khi quit), IPC
  `backend:getConfig` qua preload, `ApiClient` (token, timeout, ApiError), `sampleHelloApi.js`, UI hiển thị
  "hello client" hoặc lỗi.
- Tài liệu: `docs/high-level-design.md` (Mermaid: component, startup, request flow, lifecycle),
  `docs/api-convention.md`, `docs/coding-convention.md`; `CLAUDE.md` thêm mục Coding convention.
- Tool: `run-backend.ps1` mặc định `--dev --port 8765`, thêm `run-backend-tests.ps1`; F5 chạy với `--dev`.

**Trao đổi user ↔ AI:**
- User yêu cầu: core giao tiếp FE ↔ BE, test API trả "hello client" có naming nhận biết là test, quy chuẩn
  cho API sau này, coding convention (header file, header function, comment chỗ khó, clean code).
- User chọn: FetchContent (không cài vcpkg), cpp-httplib (WebSocket để ADR sau), dựng GoogleTest ngay,
  comment tiếng Anh.
- User yêu cầu thêm tài liệu high level design mô tả giao tiếp FE ↔ BE, chart bằng Mermaid; và không cần nhắc lại
  rule UTF-8/CRLF trong plan vì đã có hook tự động.
- User yêu cầu đổi tên `test API` thành `sample API`: URL `/api/v1/sample/*`, tag `sample`, thư mục `sample/`,
  prefix `Sample`/`sample`, option CMake `SYSTEMLENS_ENABLE_SAMPLE_API` (tránh nhầm với test suite thật).
- Trạng thái lúc commit: user sửa greeting thành "hello client 2222" để thử, nên API test
  `SampleHelloApiTest.ReturnsHelloClientWithValidToken` fail (9/10 pass); sẽ trả về "hello client" ở commit sau.
  Phần mock data cho FE (Prism từ OpenAPI) đã thống nhất thiết kế nhưng tạm pending.
- Kiểm thử (trước khi đổi greeting): 10/10 test pass; curl dev mode trả `{"message":"hello client"}`; Electron hiển thị "hello client";
  đóng app hoặc kill Electron cưỡng bức → backend tự thoát; thiếu backend exe → UI hiện lỗi rõ ràng.

## 2026-10-15 — Enforce định dạng file UTF-8 (không BOM) + CRLF

**Lý do:** Thống nhất định dạng cho mọi file được commit: UTF-8 không BOM và CRLF, ở cả repo lẫn working tree.
Kiểm tra tự động để không phụ thuộc vào việc người hoặc AI tự nhớ.

**Nội dung:**
- `Tool/check_utf8.ps1`, `Tool/check_crlf.ps1` (+ `Tool/lib/FileFormat.ps1`): kiểm tra / sửa (`-Fix`) theo file,
  toàn repo hoặc file đã stage (`-Staged`); bỏ qua file binary và file rỗng. File không phải UTF-8 được decode
  bằng code page 1252 khi `-Fix`.
- `.gitattributes` `* -text`: git lưu nguyên byte, ghi đè `core.autocrlf=true` của máy.
- Claude Code hook cấp project (`.claude/settings.json` + `.claude/hooks/fix-format.ps1`): sau mỗi Write/Edit
  tự chuyển file sang UTF-8 không BOM + CRLF. Không đụng vào cấu hình global.
- Git pre-commit hook (`Tool/githooks/pre-commit` + `.ps1`): nếu file stage sai định dạng thì hiện dialog Yes/No
  liệt kê các file; Yes = vẫn commit, No = hủy. Bật cho repo này bằng `Tool/setup-hooks.ps1` (`core.hooksPath`, local).
- Chuẩn hóa toàn bộ file hiện có từ LF sang CRLF.
- `CLAUDE.md`: thêm rule định dạng file, các tool và hook mới.

**Trao đổi user ↔ AI:**
- User yêu cầu rule UTF-8 + CRLF, 2 tool `check_utf8` / `check_crlf`; AI phải chạy 2 tool sau khi sửa / thêm file,
  sai thì convert.
- AI phát hiện `core.autocrlf=true` sẽ đổi CRLF thành LF khi commit → User chọn lưu CRLF ở cả repo
  (`* -text`), UTF-8 không BOM.
- User chọn cả 2 cơ chế: Claude Code hook (chỉ áp dụng cho workspace này, tuyệt đối không global) và git
  pre-commit hook có dialog hỏi tiếp tục commit hay không.
- Kiểm thử: tool phát hiện đúng BOM, byte cp1252, LF/CR đơn; hook Claude tự chuyển file mới sang CRLF;
  pre-commit trong repo tạm: file sai → dialog (chọn No → commit bị hủy), file đúng → commit thẳng, không có dialog.
  Backend build/chạy và frontend mở được sau khi đổi sang CRLF.

## 2026-10-15 — Setup: khởi tạo project frontend + backend

**Lý do:** Hoàn thành Bước 1 (Setup) của kế hoạch: có 2 project chạy được làm nền cho các bước thiết kế
(spec, HLD, API contract) và code sau này.

**Nội dung:**
- Cấu trúc repo: `docs/` (adr, lld), `api/`, `backend/`, `frontend/`, `Tool/`.
- `CLAUDE.md`: layout, lệnh build/chạy, quy tắc làm việc (contract-first, human gates, hỏi trước khi cài đặt,
  review code thủ công, test FE thủ công, backend có API test + interaction test).
- Backend: `CMakeLists.txt` (C++20, MSVC VS 2019) + `main.cpp` in ra "hello backend".
- Debug backend trong VS Code: `.vscode/tasks.json` (configure + build Debug), `launch.json` ("Debug backend",
  cppvsdbg), `settings.json` (CMake Tools).
- Frontend: Electron (contextIsolation, sandbox, CSP) hiển thị "hello frontend"; `package-lock.json` khóa Electron v44.7.2.
- `Tool/run-backend.ps1`, `Tool/run-frontend.ps1`: chạy được từ bất kỳ thư mục nào.
- `.gitignore` theo pattern (build output C++/CMake, VS/VS Code local state, node_modules, logs).
- `HISTORY.md` (file này).

**Trao đổi user ↔ AI:**
- User đặt mục tiêu: xây app giống MiTeC Task Manager DeLuxe để học Claude Code end-to-end (human in loop),
  hiểu thông tin Task Manager, luyện C++, học thiết kế HLD → LLD, học viết spec. Bắt buộc: FE Electron,
  BE C++, giao tiếp qua API, thiết kế API schema trước để làm FE/BE song song.
- AI đề xuất plan: kiến trúc Electron Main/Preload/Renderer ↔ C++ backend (API layer, Services, Sampling
  Engine, Action Executor, Platform Abstraction Win32/Fake); REST + WebSocket trên 127.0.0.1 có token;
  OpenAPI 3.1 + mock Prism; lộ trình 12 phase.
- User chỉnh plan: bỏ clang-format/clang-tidy/`/W4` (review thủ công); bỏ Vitest/Playwright (FE test thủ công);
  backend cần API test + interaction test; lộ trình tạm chỉ còn Bước 1 Setup; cần hỏi trước khi cài đặt.
- AI kiểm tra môi trường: có CMake 4.4, Node 24, VS 2019 (MSVC 14.29); VS 2022 thiếu C++ workload;
  VS Code thiếu extension C++. User chọn cài `ms-vscode.cpptools` + `ms-vscode.cmake-tools` để debug trong VS Code.
- User gặp lỗi khi chạy cmake trong thư mục `frontend/` (đường dẫn tương đối) → AI ghi chú "chạy từ repo root"
  trong CLAUDE.md; user yêu cầu thêm thư mục `Tool/` với script chạy FE/BE từ bất kỳ đâu.
- User yêu cầu rà soát file cần commit và bổ sung `.gitignore` theo pattern; yêu cầu tạo `HISTORY.md`.
- Chưa xác minh: debug F5 trong VS Code do user tự thử.
