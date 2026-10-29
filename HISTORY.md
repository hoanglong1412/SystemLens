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

## 2026-10-29 — Gộp tài liệu design của RQ001 còn 2 file: Architecture và api.md

**Lý do:** User thấy bộ design (HLD + 2 LLD + api-schema + api-convention) quá nhiều và phức tạp so với requirement RQ001 chỉ có 1 FR + 3 NFR. Mục đích của design là giúp người đọc hiểu phần core: có bao nhiêu component, contract thế nào, các flow chính chạy ra sao, thêm 1 API làm thế nào, các module giao tiếp với nhau ra sao.

**Nội dung:**
- Thêm `docs/design/RQ001_CoreProject/RQ001_Architecture.md` thay cho `RQ001_HLD.md`, `RQ001_LLD-01`, `RQ001_LLD-02`: 1 sơ đồ component tổng (3 liên kết A/B/C giữa các process) + bảng handshake, code map, 3 flow (start / request / end) mỗi flow 1 sequence, 1 bảng lỗi chung, bảng layer, design decisions D1–D12 (giữ nguyên số), mapping requirement. Từ 14 sơ đồ còn 4.
- Thêm `docs/api.md` thay cho `api-schema.md` và `api-convention.md`: các bước thêm API và mẫu controller đặt lên đầu, sau đó là rule và danh sách endpoint. `api/openapi.yaml` vẫn là nguồn chính.
- `docs/README.md`: rút gọn còn bảng "muốn biết gì thì đọc file nào", cây thư mục, bảng RQ, bảng requirement → design → code; bỏ ma trận trace dài, bảng ID và các header 5 dòng.
- Sửa tham chiếu `docs/api-convention.md` → `docs/api.md` trong comment của `api/openapi.yaml`, `backend/src/api/ApiResponse.h`, `frontend/renderer/api/apiClient.js` (chỉ comment) và link trong `RQ001_CoreProject.md` (chỉ link, không đổi nội dung requirement).
- Sửa 2 chỗ HLD cũ nói chưa chính xác: D9 (chỉ 404 và exception được server tự tạo problem body, lỗi khác của cpp-httplib không có body, ApiClient xử lý bằng nhánh "không có problem body") và D5/stop (kill() ngay sau khi đóng stdin nên watchdog có thể không chạy).

**Trao đổi user ↔ AI:**
- User hỏi: các file design hiện có quá nhiều/phức tạp so với requirement không, có nên gộp hoặc trình bày cách khác không.
- AI nhận xét: cùng một flow bị nói 3 lần (overview, detail, sequence) rồi lặp lại ở LLD; contract viết 2 lần (yaml và api-schema); cách thêm API nằm xa phần design. Đề xuất gộp còn 3 file chính (requirement, architecture, api). User đồng ý và yêu cầu làm trên nhánh `claude/hld-hien-tai-review-eqf1qo`.
- Kiểm tra: link/anchor trong docs đúng, 4 diagram Mermaid render được (Mermaid 12), UTF-8 + CRLF (kiểm bằng `file`, chưa chạy được `Tool\check_*.ps1` vì môi trường cloud không có PowerShell).
- Chưa làm (đã nêu trong review, chờ quyết định): thiết kế cho NFR-03, trigger "backend chết sau READY", góc nhìn build/deploy.

## 2026-10-30 — RQ001_HLD.md tổ chức theo trigger, đi từ overview đến detail

**Lý do:** User muốn HLD dễ đọc theo hướng từ tổng quan tới chi tiết, chia theo các trigger chính của RQ001.

**Nội dung:**
- `docs/design/RQ001_CoreProject/RQ001_HLD.md`: chia thành 3 trigger — HLD-01 Start app, HLD-02 End app,
  HLD-03 Request API from BE and display in FE. Mỗi trigger gồm: overview component (Frontend / IPC / Backend,
  thêm API contract `api/openapi.yaml` ở HLD-03) → detail component (các phần tử trong FE và BE) → sequence.
  Mọi diagram có bảng giải thích theo số. Design decisions chuyển thành HLD-04, mapping requirement thành HLD-05.
- `RQ001_LLD-01`, `RQ001_LLD-02`, `docs/README.md`: cập nhật "Traces to" và traceability matrix theo ID HLD mới.
- `docs/CLAUDE.md`: user rút gọn quy tắc (chỉ ghi thông tin đã xác minh, cây thư mục, quy tắc diagram, kiểm tra).

**Trao đổi user ↔ AI:**
- User yêu cầu: HLD đi từ overview đến detail, chia 3 trigger (start app, end app, request API và hiển thị), mỗi
  trigger theo thứ tự overview component (FE, BE, IPC, DB) → detail component → sequence.
- AI hỏi: code chưa có database thì block DB xử lý sao. User: DB ở đây là API / `api/openapi.yaml` nếu cần →
  chỉ vẽ ở trigger request API. User chọn giữ design decisions và mapping, đặt sau 3 trigger.
- AI đổi detail component của Start app sang dạng dọc vì dạng ngang quá rộng, khó đọc.
- Kiểm tra: link/anchor đúng, 17 diagram trong design render được (Mermaid 12.2.0), định dạng UTF-8/CRLF hợp lệ.

## 2026-10-29 — Tổ chức lại docs/ theo work item RQ001, thêm quy tắc viết tài liệu

**Lý do:** Gom yêu cầu và design theo từng work item (RQ<NNN>) để dễ trace và dễ thêm work item mới;
ghi lại quy tắc viết tài liệu ngay trong `docs/`.

**Nội dung:**
- `docs/requirements/RQ001_CoreProject.md` thay cho `functional-requirements.md` và `non-functional-requirements.md`
  (ID mới dạng `RQ001-FR-01`, `RQ001-NFR-01..03`).
- `docs/design/RQ001_CoreProject/`: `RQ001_HLD.md`, `RQ001_LLD-01-backend-lifecycle.md`,
  `RQ001_LLD-02-api-request-pipeline.md` thay cho `high-level-design.md` và `lld/` (LLD-03 sample hello gộp
  vào LLD-02 §6, bỏ `_template.md`).
- `docs/README.md`: cây thư mục, bảng ID, bảng requirement file và traceability matrix theo ID mới.
- `docs/api-schema.md`, `docs/api-convention.md`: cập nhật link sang ID / file mới.
- `docs/CLAUDE.md` (mới): quy tắc viết tài liệu trong `docs/`.
- `docs/ui-design/` giữ nguyên `.gitkeep`; link Figma (`figma.onl`) không nằm ở nhánh này.

**Trao đổi user ↔ AI:**
- User: commit phần tài liệu trong working tree lên nhánh `feature/init`, không commit phần UI; phần UI commit
  riêng lên nhánh `feature/int_cpu_preformance`. Sau đó: nhánh này không cần `docs/ui-design/` (giữ `.gitkeep`),
  link Figma và `evi/` commit ở nhánh UI.
- AI: tách commit theo thư mục — `docs/` (và HISTORY.md) ở commit này; `frontend/`, `docs/ui-design/figma.onl`,
  `evi/` ở nhánh UI.

## 2026-10-28 — Tạo lại bộ tài liệu docs/ (yêu cầu, HLD, LLD, API schema) theo source hiện tại

**Lý do:** Bộ docs cũ đã bị xoá để tái cấu trúc. User muốn bộ tài liệu mới dễ đọc, dễ trace từ yêu cầu → design
→ API → source, và chỉ ghi theo nội dung source code hiện tại (không tự thêm yêu cầu hay đề xuất).

**Nội dung:**
- `docs/README.md`: mục lục, quy ước ID (`FR-`, `UC-`, `NFR-`, `HLD-`, `LLD-`, `operationId`), ma trận traceability.
- `docs/requirements/functional-requirements.md`: `FR-SMP-01` / `UC-SMP-01` (hiển thị greeting từ sample API),
  bảng thao tác UI, template cho FR mới.
- `docs/requirements/non-functional-requirements.md`: 3 NFR do user đưa ra — NFR-01 API contract là source of
  truth (Implemented), NFR-02 tách biệt IPC FE ↔ BE (Implemented), NFR-03 BE độc lập OS (Not implemented, chỉ
  liệt kê các chỗ đang phụ thuộc Windows); kèm bằng chứng trong code và template.
- `docs/design/high-level-design.md`: HLD-01 component diagram, HLD-02 luồng UI → BE → hiển thị, HLD-03 start app,
  HLD-04 end app, HLD-05 gọi 1 API. Mọi sequence dùng `autonumber` và có bảng mô tả từng bước.
- `docs/design/lld/`: `_template.md`, LLD-01 vòng đời backend, LLD-02 pipeline request API, LLD-03 sample hello;
  mỗi LLD có sequence mức hàm, bảng hàm (params/return/lỗi) và flowchart các hàm có rẽ nhánh.
- `docs/api-schema.md`: bảng API sinh từ `api/openapi.yaml` (YAML là chuẩn khi lệch); `docs/api-convention.md`:
  quy ước API đang áp dụng trong code (đường dẫn này đang được `openapi.yaml`, `ApiResponse.h`, `apiClient.js` tham chiếu).
- `docs/ui-design/.gitkeep`: thư mục chờ file thiết kế Figma.

**Trao đổi user ↔ AI:**
- User yêu cầu: file yêu cầu chức năng/phi chức năng, HLD, LLD, API schema dạng bảng, thư mục Figma; diagram
  bằng Mermaid, sequence đánh số kèm bảng; tài liệu dễ mapping; không tự thêm yêu cầu; file khác cần thiết thì hỏi.
- AI hỏi, user chọn: FR ghi lại chức năng sample hiện có; NFR chỉ gồm 3 mục của user; tạo thêm `docs/README.md`
  và `docs/api-convention.md` (không tạo `coding-convention.md`); viết bằng tiếng Anh.
- User bổ sung: cây thư mục trong tài liệu dùng ký hiệu `├──`, `└──`, `│`.
- Trước khi commit, AI phát hiện commit "Move C++ function header comments" đã xoá `HttpServer::waitUntilReady()` và
  đổi vài comment liên quan tới test; đã cập nhật LLD-02, LLD-03 cho khớp.
- Kiểm tra: UTF-8/CRLF hợp lệ, toàn bộ link/anchor trong `docs/` đúng, 34 diagram Mermaid render không lỗi.

## 2026-10-27 — Chuyển comment header của hàm C++ từ .h sang .cpp

**Lý do:** User muốn file `.h` chỉ còn khai báo; comment mô tả hàm (Doxygen) đặt ở phần cài đặt trong `.cpp`.

**Nội dung:**
- Chuyển nguyên văn comment hàm (chỉ bỏ thụt lề) từ `.h` sang `.cpp` cho `ApiResponse`, `HttpServer`,
  `SampleHelloController`, `AppConfig`, `Application`, `SampleHelloService`.
- Giữ ở `.h`: comment hàm pure virtual của `IController`, `IRouteRegistry`, `ISampleHelloService` (không có
  `.cpp`), comment file header, struct, hằng số và biến thành viên.
- `CLAUDE.md`: rule comment hàm đổi thành Doxygen ở `.cpp`, không comment hàm trong `.h` (user sửa), kèm ngoại
  lệ cho hàm pure virtual của interface (AI thêm theo yêu cầu); user bỏ dòng link tới `docs/coding-convention.md`.

**Trao đổi user ↔ AI:**
- User yêu cầu chuyển toàn bộ comment header của hàm từ `.h` sang `.cpp`, không đổi nội dung.
- AI hỏi 2 điểm: interface pure virtual không có `.cpp` → user chọn giữ ở `.h`; rule cũ trong `CLAUDE.md`
  ngược với yêu cầu → user tự sửa rule, sau đó yêu cầu AI bổ sung ngoại lệ cho interface.
- Kiểm thử: build Debug backend không lỗi/warning; định dạng các file sửa hợp lệ.

## 2026-10-27 — Tạm gỡ bỏ test backend để tái cấu trúc core và tài liệu

**Lý do:** User muốn tái cấu trúc hệ thống, làm xong phần core và tài liệu trước rồi mới làm test, nên tạm
gỡ toàn bộ test hiện có.

**Nội dung:**
- Xoá thư mục `backend/tests/` (API test, interaction test gmock, `FakeRouteRegistry`) và script
  `Tool/run-backend-tests.ps1`.
- `backend/CMakeLists.txt`: bỏ option `SYSTEMLENS_BUILD_TESTS`, khối `enable_testing()` / `add_subdirectory(tests)`
  và các comment nhắc tới test.
- `backend/src`: xoá `HttpServer::waitUntilReady()` (chỉ test dùng); sửa comment nhắc tới test trong
  `HttpServer.h`, `IController.h`, `IRouteRegistry.h`, `ISampleHelloService.h` (vẫn giữ các interface).
- `README.md`: bỏ `googletest`, lệnh `run-backend-tests.ps1` và `ctest`. `CLAUDE.md`: bỏ mục mô tả 2 bộ test.

**Trao đổi user ↔ AI:**
- User yêu cầu review `CLAUDE.md` và `README.md` (chỉ đọc 2 file); AI đưa danh sách điểm cần sửa/bổ sung.
- User yêu cầu xoá `backend/tests` và thông tin liên quan tới test. AI xoá, sửa CMake/README/CLAUDE.md và báo
  còn sót `waitUntilReady` cùng các comment trong `backend/src`; user chọn dọn luôn.
- User tự bỏ dòng rule "No automated tests for now" mà AI thêm vào `CLAUDE.md`.
- Kiểm thử: build Debug bằng `Tool\run-backend.ps1` không lỗi/warning; backend lên port 8765,
  `GET /api/v1/sample/hello` trả `{"message":"@@Hello world@@"}`.

## 2026-10-16 — Tài liệu Spec → HLD → LLD, API reference, greeting "@@Hello world@@"

**Lý do:** Hoàn thiện chuỗi tài liệu thiết kế (spec tổng → HLD → LLD) và bảng tra cứu API; đồng bộ greeting
mới user đặt giữa contract, code, test và tài liệu.

**Nội dung:**
- `docs/SystemLens_spec.md` (mới, v0.2): spec tổng của dự án — tính năng + trạng thái, FR-COM/FR-SMP, NFR,
  acceptance criteria (Given/When/Then), traceability, vấn đề đã biết, lịch sử spec.
- `docs/high-level-design.md`: component diagram đánh số ①–⑨ kèm bảng bước; sequence Startup / Request flow
  tách FE/BE bằng `box` màu và có bảng bước (khớp `autonumber`); mục Shutdown đổi sang sequence, bỏ actor
  "Người dùng / OS".
- `docs/lld/backend-lld.md`, `docs/lld/frontend-lld.md` (mới): class diagram, đặc tả thành phần, threading,
  lỗi/exit code, IPC, pipeline ApiClient, công thức thêm API.
- `docs/api-reference.md` (mới): mỗi API một dòng, tóm tắt từ `api/openapi.yaml`; checklist trong
  `api-convention.md` thêm bước cập nhật file này.
- Greeting đổi thành `"@@Hello world@@"` (user) ở contract, `SampleHelloService`, API test và tài liệu.
- `CLAUDE.md` chuyển phần Layout/Commands sang `README.md` (user), thêm rule "Spec first".

**Trao đổi user ↔ AI:**
- User yêu cầu: đánh số component diagram, tách xử lý FE/BE trong sequence, đổi lifecycle sang sequence,
  thêm 2 LLD; sau đó yêu cầu thêm spec tổng `SystemLens_spec` (không chia file theo tính năng); thêm bảng bước
  cho mục 3, 4; bỏ "Người dùng / OS" ở mục 5; thêm file mô tả API dạng bảng.
- Trước khi commit, AI phát hiện đợt thay "hello client" → "@@Hello world@@" gây 3 lỗi: `openapi.yaml` sai cú
  pháp YAML (plain scalar không được bắt đầu bằng `@`), API test mong đợi `"@@Hello world@@222"` nên fail, và
  entry HISTORY đã commit bị sửa. User chọn sửa trước khi commit: đặt greeting trong dấu nháy, test khớp
  greeting mới, trả HISTORY cũ về nguyên trạng, cập nhật spec I-1 thành "Đã xử lý".
- Kiểm thử: 10/10 test backend pass; 9 chart Mermaid (HLD + 2 LLD) render OK; định dạng toàn repo hợp lệ.

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
