# SystemLens — Specification

> Spec tổng của dự án. Mọi tính năng mới bắt đầu bằng việc cập nhật file này, sau đó mới tới contract
> ([api/openapi.yaml](../api/openapi.yaml)), thiết kế ([high-level-design.md](high-level-design.md),
> [lld/backend-lld.md](lld/backend-lld.md), [lld/frontend-lld.md](lld/frontend-lld.md)) và code.
>
> Phiên bản: 0.2 · Cập nhật: 2026-10-16

## 1. Giới thiệu

### 1.1. Mục đích

SystemLens là ứng dụng quản lý tác vụ (Task Manager) cho Windows, tham khảo
[MiTeC Task Manager DeLuxe](https://www.mitec.cz/wp/tmx/). Ứng dụng hiển thị và điều khiển process, hiệu năng,
service, kết nối mạng, chương trình khởi động… của máy.

### 1.2. Mục tiêu dự án

- Hiểu rõ các thông tin hệ điều hành mà Task Manager hiển thị (CPU, bộ nhớ, handle, thread…).
- Luyện C++ (backend) và thiết kế ứng dụng vừa–lớn từ HLD tới LLD.
- Luyện viết spec rõ ràng và quy trình làm việc với AI (Claude Code) có con người review (human in the loop).

### 1.3. Người dùng

Người dùng Windows có kiến thức kỹ thuật (developer, power user) muốn xem và điều khiển trạng thái máy.

### 1.4. Phạm vi

| Trong phạm vi | Ngoài phạm vi (hiện tại) |
|---|---|
| Ứng dụng desktop Windows 10/11 x64 | macOS / Linux |
| Frontend Electron + backend C++ chạy local | Giám sát máy từ xa, nhiều người dùng |
| Giao tiếp qua HTTP API local (loopback) | Cài đặt/cập nhật tự động, telemetry |

## 2. Thuật ngữ

| Thuật ngữ | Ý nghĩa |
|---|---|
| **FE / BE** | Frontend (Electron) / Backend (`systemlens-backend.exe`, C++) |
| **Handshake** | Quá trình FE khởi động BE và nhận lại port BE đang nghe |
| **READY line** | Dòng `SYSTEMLENS_READY {"port":<n>}` BE in ra stdout khi sẵn sàng phục vụ |
| **Token** | Chuỗi ngẫu nhiên 256-bit FE sinh ra mỗi lần chạy; mọi request phải gửi kèm `Authorization: Bearer <token>` |
| **Dev mode** | BE chạy với `--dev`: tắt auth để test bằng curl / debug |
| **Sample API** | API mẫu dưới `/api/v1/sample/*`: kiểm tra hạ tầng FE ↔ BE và làm ví dụ cho API thật |
| **problem+json** | Định dạng lỗi theo RFC 7807 (`application/problem+json`) |
| **CORS preflight** | Request `OPTIONS` trình duyệt tự gửi trước một request cross-origin có header `Authorization` |
| **Contract** | `api/openapi.yaml` — nguồn sự thật chung của FE và BE |

## 3. Tổng quan tính năng

| Mã | Tính năng | Trạng thái | Ghi chú |
|---|---|---|---|
| **COM** | Giao tiếp FE ↔ BE: khởi động, handshake, auth, lỗi, vòng đời | ✅ Done | Mục 4.1 |
| **SMP** | Sample API `/api/v1/sample/hello` | ✅ Done | Mục 4.2 |
| **MCK** | FE chạy với mock data khi BE chưa có API | ⏸ Pending | Mục 4.3 |
| **PRC** | Danh sách process, chi tiết, kill / priority / affinity | 📝 Planned | |
| **PRF** | Hiệu năng: CPU, bộ nhớ, disk, network theo thời gian thực | 📝 Planned | Cần quyết định stream realtime |
| **SVC** | Windows services: xem, start / stop | 📝 Planned | |
| **NET** | Kết nối mạng (TCP/UDP) theo process | 📝 Planned | |
| **STU** | Chương trình khởi động cùng Windows | 📝 Planned | |
| **SYS** | Thông tin hệ thống (OS, CPU, RAM, uptime) | 📝 Planned | |

Ký hiệu trạng thái: ✅ Done · 🚧 In progress · ⏸ Pending · 📝 Planned (mới liệt kê, chưa viết yêu cầu chi tiết).

## 4. Functional requirements

### 4.1. COM — Giao tiếp FE ↔ BE

| ID | Yêu cầu |
|---|---|
| **FR-COM-01** | Khi app khởi động, FE tự chạy BE với `--port 0` (OS chọn port trống) và một token ngẫu nhiên mới truyền qua biến môi trường `SYSTEMLENS_TOKEN`. |
| **FR-COM-02** | BE chỉ nghe trên `127.0.0.1`. Khi sẵn sàng, BE in **đúng một** dòng READY ra stdout; mọi log khác đi ra stderr. |
| **FR-COM-03** | Renderer lấy `{ baseUrl, token }` qua `window.systemlens.getBackendConfig()` (IPC `backend:getConfig`), không truy cập Node/Electron trực tiếp. |
| **FR-COM-04** | Mọi request (trừ preflight) thiếu hoặc sai token bị từ chối với `401`. Preflight `OPTIONS` được trả `204` kèm CORS header, không cần token. |
| **FR-COM-05** | Mọi lỗi trả về dạng problem+json: route không tồn tại → `404`; exception trong controller/service → `500`, `detail` chứa thông điệp lỗi. |
| **FR-COM-06** | BE không bao giờ sống lâu hơn FE: khi app đóng bình thường hoặc Electron crash, BE tự thoát. |
| **FR-COM-07** | Nếu BE không khởi động được (thiếu exe, thoát sớm, quá 10 giây chưa READY), cửa sổ vẫn mở và hiển thị lỗi rõ ràng. |
| **FR-COM-08** | BE chạy với `--dev` thì tắt auth và in cảnh báo ra stderr. Thiếu `SYSTEMLENS_TOKEN` mà không có `--dev` thì BE in hướng dẫn và thoát với mã `2`. |

### 4.2. SMP — Sample API

| ID | Yêu cầu |
|---|---|
| **FR-SMP-01** | `GET /api/v1/sample/hello` trả `200` với body `{"message":"@@Hello world@@"}` (`application/json`). FE hiển thị message này trên giao diện. |
| **FR-SMP-02** | Sample API chỉ được build và đăng ký khi CMake option `SYSTEMLENS_ENABLE_SAMPLE_API=ON` (mặc định ON trong giai đoạn phát triển). |

### 4.3. MCK — FE chạy với mock data *(Pending)*

Thiết kế đã thống nhất, chưa triển khai:

- Mock server **Prism** sinh response từ `example` trong `api/openapi.yaml`, nên mock luôn khớp contract.
- Biến môi trường `SYSTEMLENS_API_MODE=mock`: FE không spawn BE, `baseUrl` trỏ tới Prism (`http://127.0.0.1:4010`).
- UI hiển thị badge "MOCK DATA" khi chạy mock.
- Quy ước: mọi response trong contract phải có `example`.

Yêu cầu chi tiết (FR-MCK-xx) sẽ viết khi bắt đầu triển khai.

## 5. Non-functional requirements

| ID | Nhóm | Yêu cầu |
|---|---|---|
| **NFR-SEC-01** | Bảo mật | API chỉ bind loopback `127.0.0.1`. |
| **NFR-SEC-02** | Bảo mật | Token 256-bit ngẫu nhiên mỗi lần chạy, truyền qua env (không qua argv), so sánh constant-time. |
| **NFR-SEC-03** | Bảo mật | Renderer chạy với `contextIsolation`, `sandbox`, `nodeIntegration: false`; CSP chỉ cho phép `connect-src http://127.0.0.1:*`. |
| **NFR-PERF-01** | Hiệu năng | BE gửi READY trong vòng 10 giây kể từ khi spawn. |
| **NFR-PERF-02** | Hiệu năng | Mỗi request từ FE có timeout 5 giây. |
| **NFR-MNT-01** | Bảo trì | Contract-first: endpoint phải có trong `api/openapi.yaml` trước khi code ([api-convention.md](api-convention.md)). |
| **NFR-MNT-02** | Bảo trì | Phân lớp HttpServer → Controller → Service (→ Provider); FE: UI → API module → ApiClient. |
| **NFR-MNT-03** | Bảo trì | Code tuân theo [coding-convention.md](coding-convention.md): header file / function, comment chỗ khó hiểu, clean code. |
| **NFR-QLT-01** | Chất lượng | Mọi file text là UTF-8 không BOM + CRLF (hook Claude + pre-commit kiểm tra). |
| **NFR-QLT-02** | Chất lượng | Mỗi API có API test (HTTP thật) và interaction test (gmock). FE được test thủ công. |

## 6. Acceptance criteria

| ID | Given | When | Then |
|---|---|---|---|
| FR-COM-01 | Đã build backend Debug | Chạy `Tool\run-frontend.ps1` | Có đúng 1 process `systemlens-backend.exe`, nghe trên một port ngẫu nhiên |
| FR-COM-02 | BE được spawn | BE bind xong | stdout chỉ có 1 dòng `SYSTEMLENS_READY {"port":n}`, log nằm ở stderr |
| FR-COM-03 | App đang chạy | Renderer gọi `getBackendConfig()` | Nhận `{ baseUrl: "http://127.0.0.1:<n>", token }` |
| FR-COM-04 | BE chạy có token | Gọi API không có / sai `Authorization` | `401` + problem+json; `OPTIONS` không token → `204` + `Access-Control-Allow-Origin: *` |
| FR-COM-05 | BE đang chạy | Gọi route không tồn tại / service ném exception | `404` / `500`, `Content-Type: application/problem+json` |
| FR-COM-06 | App đang chạy | Đóng cửa sổ, hoặc kill Electron cưỡng bức | Trong vài giây không còn process `systemlens-backend.exe` |
| FR-COM-07 | Exe backend không tồn tại | Mở app | Cửa sổ hiện "Cannot reach backend: …", không trắng trang |
| FR-COM-08 | Không có `SYSTEMLENS_TOKEN` | Chạy exe không `--dev` / có `--dev` | Thoát mã `2` kèm usage / chạy bình thường và in cảnh báo auth disabled |
| FR-SMP-01 | App đang chạy bình thường | Mở cửa sổ | Hiển thị "@@Hello world@@"; API trả `{"message":"@@Hello world@@"}` |
| FR-SMP-02 | Configure với `-DSYSTEMLENS_ENABLE_SAMPLE_API=OFF` | Gọi `/api/v1/sample/hello` | `404` (route không được đăng ký) |

## 7. Traceability

| Yêu cầu | Contract | Thiết kế | Test tự động | Test thủ công |
|---|---|---|---|---|
| FR-COM-01 | — | HLD §3, LLD-FE §3 | — | Chạy app, kiểm tra process |
| FR-COM-02 | `servers` | HLD §3, LLD-BE §5 | — (gián tiếp: mọi API test cần server sẵn sàng) | Chạy exe, xem stdout/stderr |
| FR-COM-03 | — | HLD §3, LLD-FE §4 | — | Chạy app |
| FR-COM-04 | `bearerAuth`, `401` | HLD §4, LLD-BE §5 | `SampleHelloApiTest.RejectsMissingToken`, `RejectsWrongToken`, `AnswersCorsPreflightWithoutToken` | — |
| FR-COM-05 | `Problem`, `404`, `500` | HLD §4, LLD-BE §5 | `SampleHelloApiTest.ReturnsProblemForUnknownRoute`, `ReturnsProblemWhenServiceThrows`, `SampleHelloControllerTest.PropagatesServiceExceptionsToTheServer` | — |
| FR-COM-06 | — | HLD §5, LLD-BE §4, LLD-FE §3 | — | Đóng app / kill Electron |
| FR-COM-07 | — | HLD §5, LLD-FE §3 | — | Đặt `SYSTEMLENS_BACKEND_EXE` sai |
| FR-COM-08 | — | LLD-BE §5 | — | Chạy exe có/không `--dev` |
| FR-SMP-01 | `/api/v1/sample/hello` | HLD §4 | `SampleHelloApiTest.ReturnsHelloClientWithValidToken`, `SampleHelloControllerTest.*` | Mở app |
| FR-SMP-02 | tag `sample` | LLD-BE §1 | — | Configure OFF |
| NFR-SEC-01..03 | — | HLD §6 | Các test 401 ở trên | Xem cấu hình CSP / webPreferences |
| NFR-PERF-01..02 | — | LLD-FE §3, §5 | — | — |
| NFR-QLT-01 | — | — | `Tool\check_utf8.ps1`, `Tool\check_crlf.ps1`, pre-commit hook | — |
| NFR-QLT-02 | — | LLD-BE §6 | `Tool\run-backend-tests.ps1` | — |

Các yêu cầu chỉ có test thủ công (FR-COM-01/03/06/07/08) là ứng viên cho test tự động sau này.

## 8. Vấn đề đã biết & câu hỏi mở

| # | Nội dung | Trạng thái |
|---|---|---|
| I-1 | Greeting từng lệch giữa code và contract. User đổi greeting thành `"@@Hello world@@"`; contract, code, API test và tài liệu đã thống nhất theo giá trị này. | Đã xử lý |
| I-2 | MCK (mock data cho FE) đã thiết kế, chưa triển khai. | Pending |
| Q-1 | Giao thức stream số liệu realtime (WebSocket / SSE) cho PRF — cpp-httplib chưa hỗ trợ WebSocket. | Chưa quyết (cần ADR) |
| Q-2 | Chạy BE với quyền admin để đọc đủ thông tin mọi process. | Chưa quyết |
| Q-3 | Cách đóng gói BE cùng app Electron và đường dẫn exe khi phát hành. | Chưa quyết |

## 9. Lịch sử thay đổi spec

| Phiên bản | Ngày | Nội dung |
|---|---|---|
| 0.1 | 2026-10-16 | Bản đầu: COM, SMP (Done), MCK (Pending), danh sách tính năng Planned, NFR, traceability. |
| 0.2 | 2026-10-16 | FR-SMP-01: greeting đổi thành `"@@Hello world@@"`; I-1 đã xử lý. |
