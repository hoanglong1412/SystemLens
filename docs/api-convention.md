# API Convention

Mọi API giữa Frontend và Backend phải tuân theo tài liệu này. Kiến trúc tổng quan xem ở
[high-level-design.md](high-level-design.md). Contract nằm ở [api/openapi.yaml](../api/openapi.yaml).

## 1. Contract first

1. Khai báo endpoint trong `api/openapi.yaml` **trước** khi code. Mọi thay đổi contract cần user duyệt.
2. Backend và frontend code song song dựa trên contract đó.
3. API test của backend kiểm tra đúng status, content-type và body như contract.

## 2. URL

- **Dạng:** `/api/v1/<resource>[/<id>][/<sub-resource>]`
- **Resource:** danh từ số nhiều, kebab-case. Ví dụ: `/api/v1/processes`, `/api/v1/processes/{pid}/threads`.
- **Hành động không phải CRUD:** dùng `POST` lên sub-path động từ, ví dụ `POST /api/v1/processes/{pid}/terminate`.
- **Version:** `v1` nằm trong path. Breaking change thì tạo `v2`, không sửa ngầm `v1`.

| Method | Dùng cho |
|---|---|
| `GET` | Đọc dữ liệu, không có side effect |
| `POST` | Tạo mới / thực thi hành động |
| `PUT` | Thay thế/cập nhật toàn bộ một thuộc tính (ví dụ priority) |
| `DELETE` | Xóa |

## 3. Sample API

Sample API là API mẫu: dùng để kiểm tra hạ tầng FE ↔ BE và làm ví dụ tham chiếu khi viết API thật,
không phải tính năng của sản phẩm. Nó được đánh dấu rõ ở mọi tầng:

| Tầng | Quy ước | Ví dụ |
|---|---|---|
| URL | Prefix `/api/v1/sample/` | `/api/v1/sample/hello` |
| OpenAPI | Tag `sample` | `tags: [sample]` |
| C++ | Thư mục `sample/`, class prefix `Sample`, namespace `...::sample`, file header ghi `[SAMPLE API]` | `api/sample/SampleHelloController`, `services/sample/SampleHelloService` |
| JS | Thư mục `api/sample/`, file/hàm chứa `sample`/`Sample` | `api/sample/sampleHelloApi.js`, `getSampleHello()` |
| Build | Chỉ được build/đăng ký khi CMake option `SYSTEMLENS_ENABLE_SAMPLE_API=ON` | `#if SYSTEMLENS_ENABLE_SAMPLE_API` |

## 4. Request & Response

- **Định dạng:** JSON UTF-8. Tên field dùng **camelCase**, ví dụ `workingSetBytes`.
- **Đơn vị** ghi rõ trong tên field: `...Bytes`, `...Ms`, `...Percent`. Thời gian dùng epoch milliseconds (`...AtMs`).
- **Thành công:** status `2xx`, `Content-Type: application/json`, body là chính resource (không bọc envelope).
  ```json
  { "message": "@@Hello world@@" }
  ```
- **Lỗi:** status `4xx/5xx`, `Content-Type: application/problem+json` theo [RFC 7807](https://www.rfc-editor.org/rfc/rfc7807):
  ```json
  { "type": "about:blank", "title": "Unauthorized", "status": 401, "detail": "Missing or invalid bearer token." }
  ```
- Backend tạo response **chỉ** qua `api::ok()` và `api::problem()` (`backend/src/api/ApiResponse.h`).
- Frontend nhận lỗi dưới dạng `ApiError` (`frontend/renderer/api/apiClient.js`), có `status` và `title`.

| Status | Khi nào |
|---|---|
| 200 | Thành công có body |
| 204 | Thành công không có body (và CORS preflight) |
| 400 | Request sai (thiếu/sai tham số) |
| 401 | Thiếu/sai bearer token |
| 404 | Route hoặc resource không tồn tại |
| 500 | Lỗi không mong đợi (exception) |

## 5. Authentication

- Mọi request (trừ preflight `OPTIONS`) phải có header `Authorization: Bearer <token>`.
- Token do Electron main sinh ra mỗi lần chạy và truyền vào backend qua biến môi trường `SYSTEMLENS_TOKEN`.
- `--dev` tắt auth. Chỉ dùng khi phát triển, ví dụ `Tool\run-backend.ps1` rồi gọi bằng `curl`.

## 6. Code layout cho một API

| Tầng | Vị trí | Ghi chú |
|---|---|---|
| Service interface | `backend/src/services/<area>/I<Name>Service.h` | Để interaction test mock được |
| Service | `backend/src/services/<area>/<Name>Service.h/.cpp` | Logic nghiệp vụ, không biết HTTP |
| Controller | `backend/src/api/<area>/<Name>Controller.h/.cpp` | `kRoute` constant, `registerRoutes()`, handler private |
| Đăng ký | `backend/src/app/Application.cpp` → `registerControllers()` | Tạo service + controller |
| FE API module | `frontend/renderer/api/<area>/<name>Api.js` | Mỗi endpoint một hàm `async`, nhận `ApiClient` |
| API test | `backend/tests/api/<Name>ApiTest.cpp` | HTTP thật, kiểm tra contract + lỗi |
| Interaction test | `backend/tests/interaction/<Name>ControllerTest.cpp` | gmock service + `FakeRouteRegistry` |

Mẫu tham chiếu: toàn bộ chuỗi file `SampleHello*` / `sampleHelloApi.js`.

## Checklist thêm API mới

1. [ ] Khai báo path, schema và error response trong `api/openapi.yaml` (user duyệt), thêm 1 dòng vào
   bảng [api-reference.md](api-reference.md).
2. [ ] Viết service interface và implementation.
3. [ ] Viết controller, rồi đăng ký trong `Application::registerControllers()`.
4. [ ] Thêm file `.cpp` vào `backend/CMakeLists.txt` (và `tests/CMakeLists.txt`).
5. [ ] Viết API test và interaction test, chạy `Tool\run-backend-tests.ps1`.
6. [ ] Viết FE api module và dùng nó trong UI.
7. [ ] Cập nhật `HISTORY.md`.
