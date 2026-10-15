# Coding Convention

Áp dụng cho mọi code trong repo (C++ backend, JS frontend, script PowerShell).
Comment trong code viết bằng **tiếng Anh**.

## 1. Bốn quy tắc bắt buộc

1. **File header:** mỗi file mở đầu bằng comment nêu tên file và nội dung/trách nhiệm của file.
2. **Function header:** mỗi function/method có comment mô tả: làm gì, tham số, giá trị trả về, lỗi có thể ném.
   - C++: viết ở **chỗ khai báo** (file `.h`). Không lặp lại ở `.cpp`. Hàm chỉ có trong `.cpp` (anonymous namespace) thì viết header ngay tại đó.
   - JS: JSDoc ngay trên function.
3. **Comment chỗ khó hiểu:** giải thích **vì sao** (lý do, ràng buộc, cái bẫy), không lặp lại code làm **gì**.
   Ví dụ tốt: `// Browsers send the preflight without the Authorization header, so it must be answered before the auth check.`
4. **Clean code, dễ maintain, dễ mở rộng:** xem mục 4.

## 2. Mẫu header

### C++ (`.h` / `.cpp`)

```cpp
/**
 * @file    ProcessService.h
 * @brief   One-line summary of what this file contains.
 *
 * Optional: longer explanation, how it fits in the architecture, important constraints.
 */
```

```cpp
/**
 * @brief  What the function does (one sentence).
 * @param  pid     Meaning of the parameter.
 * @return Meaning of the return value.
 * @throws std::invalid_argument When ...
 */
ProcessInfo getProcess(uint32_t pid) const;
```

### JavaScript

```js
/**
 * @file   processApi.js
 * @brief  One-line summary of what this file contains.
 */
```

```js
/**
 * What the function does (one sentence).
 * @param {ApiClient} client Configured API client.
 * @param {number} pid Process id.
 * @returns {Promise<object>} Meaning of the return value.
 * @throws {ApiError} When ...
 */
export async function getProcess(client, pid) { ... }
```

### PowerShell / CMake / YAML / HTML / CSS

Dùng comment mở đầu file (`#`, `<!-- -->`, `/* */`) nêu tên file và mục đích.

## 3. Naming

### C++

| Thành phần | Quy ước | Ví dụ |
|---|---|---|
| Class / struct / enum | `PascalCase` | `HttpServer`, `HttpMethod` |
| Interface | `I` + `PascalCase` | `ISampleHelloService` |
| Function / method / biến | `camelCase` | `registerRoutes()`, `bindPort` |
| Member | `m_` + `camelCase` | `m_server` |
| Hằng số | `k` + `PascalCase` | `kRoute`, `kReadyPrefix` |
| Namespace | `systemlens::<layer>[::<area>]` | `systemlens::api::sample` |
| File | Trùng tên class, mỗi class một cặp `.h/.cpp` | `HttpServer.h`, `HttpServer.cpp` |
| Header guard | `#pragma once` | |

### JavaScript

| Thành phần | Quy ước | Ví dụ |
|---|---|---|
| File | `camelCase.js` | `apiClient.js`, `backendProcess.js` |
| Class | `PascalCase` | `ApiClient`, `BackendProcess` |
| Function / biến | `camelCase` | `getSampleHello` |
| Hằng số module | `UPPER_SNAKE_CASE` | `SAMPLE_HELLO_PATH` |
| Private member | `#name` | `#request()` |

## 4. Clean code

- **Một trách nhiệm:** mỗi class/hàm làm một việc. Hàm dài hơn khoảng 40 dòng là dấu hiệu nên tách.
- **Phụ thuộc vào interface, inject qua constructor:** ví dụ controller nhận `shared_ptr<const IXxxService>`, không tự `new` service. Nhờ vậy test mock được.
- **Tôn trọng layering** trong [high-level-design.md](high-level-design.md#7-layering--mở-rộng): Controller không gọi Win32, Service không biết HTTP, UI không gọi `fetch` trực tiếp.
- **Không magic number/string:** đặt tên hằng số, ví dụ `status::kUnauthorized`, `READY_PREFIX`.
- **Xử lý lỗi rõ ràng:**
  - C++ ném exception chuẩn (`std::invalid_argument`, `std::runtime_error`); HttpServer chuyển chúng thành 500.
  - JS ném `ApiError` / `Error` có message cụ thể.
  - Không nuốt lỗi im lặng.
- **`const` & ownership (C++):**
  - `const` cho mọi thứ không thay đổi.
  - `std::unique_ptr` / `std::shared_ptr`, không dùng `new`/`delete` trần.
  - RAII cho tài nguyên.
- **Không lặp code:** logic dùng chung đưa vào helper (ví dụ `ApiResponse.h`, `apiClient.js`).
- **Mở rộng bằng cách thêm, không sửa:** API mới chỉ thêm file mới và một dòng đăng ký, không sửa hạ tầng (`HttpServer`, `ApiClient`).
- **Test đi kèm code:** mỗi API mới có API test và interaction test (xem [api-convention.md](api-convention.md)).
- **stdout của backend chỉ dành cho handshake** (`SYSTEMLENS_READY`); log ghi ra `stderr`.

## 5. Format

- **Thụt lề:** C++ 4 spaces, ngoặc nhọn xuống dòng (Allman); JS/JSON/YAML/HTML/CSS 2 spaces.
- **Độ dài dòng:** khoảng ≤ 120 ký tự.
- **Include C++** theo thứ tự: header tương ứng của file → header trong project → thư viện bên thứ 3 → thư viện chuẩn; mỗi nhóm cách nhau một dòng trống.
- **Định dạng file** (UTF-8 không BOM + CRLF) được hook kiểm tra tự động.
