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
