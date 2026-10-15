# SystemLens — API Reference

> Bảng tra cứu API, tóm tắt từ contract [api/openapi.yaml](../api/openapi.yaml) (phiên bản contract **0.1.0**).
> **Contract là nguồn sự thật**: khi `openapi.yaml` thay đổi, file này phải được cập nhật trong cùng commit.
> Quy ước chung: [api-convention.md](api-convention.md).

## 1. Danh sách API

Mỗi API một dòng.

| # | Method | Path | operationId | Tag | Mô tả | Auth | Request | Response thành công | Response lỗi | Ghi chú |
|---|---|---|---|---|---|---|---|---|---|---|
| 1 | `GET` | `/api/v1/sample/hello` | `getSampleHello` | `sample` | Trả lời chào cố định để kiểm tra giao tiếp FE ↔ BE | Bearer | Không có tham số, không có body | `200` `application/json` — [`SampleHelloResponse`](#samplehelloresponse), ví dụ `{"message":"@@Hello world@@"}` | `401` [Unauthorized](#3-response-lỗi-dùng-chung) · `500` [InternalError](#3-response-lỗi-dùng-chung) | Sample API: chỉ có khi build với `SYSTEMLENS_ENABLE_SAMPLE_API=ON` |

## 2. Thông tin chung

| Mục | Giá trị |
|---|---|
| Base URL | `http://127.0.0.1:{port}`. Port ngẫu nhiên khi chạy thật (`--port 0`, báo qua dòng `SYSTEMLENS_READY`); `8765` khi chạy bằng `Tool/run-backend.ps1` |
| Xác thực (`bearerAuth`) | Header `Authorization: Bearer <token>` cho mọi API. Token do Electron main sinh mỗi lần chạy, truyền cho backend qua env `SYSTEMLENS_TOKEN`. Không cần khi backend chạy `--dev` |
| Định dạng | JSON UTF-8; lỗi dùng `application/problem+json` (RFC 7807) |
| Preflight CORS | `OPTIONS` trên mọi path trả `204` kèm `Access-Control-Allow-*`, không cần token (không khai báo trong contract) |

## 3. Response lỗi dùng chung

| Tên trong contract | Status | Khi nào | Body |
|---|---|---|---|
| `Unauthorized` | `401` | Thiếu hoặc sai bearer token | [`Problem`](#problem) |
| `NotFound` | `404` | Không có route khớp path | [`Problem`](#problem) |
| `InternalError` | `500` | Lỗi không mong đợi phía server | [`Problem`](#problem) |

## 4. Schemas

### SampleHelloResponse

| Field | Kiểu | Bắt buộc | Mô tả |
|---|---|---|---|
| `message` | string | ✔ | Luôn là `"@@Hello world@@"` |

### Problem

Body lỗi theo RFC 7807 (`application/problem+json`).

| Field | Kiểu | Bắt buộc | Mô tả |
|---|---|---|---|
| `type` | string | ✔ | URI định danh loại lỗi; `"about:blank"` khi là lỗi chung |
| `title` | string | ✔ | Tóm tắt ngắn, ví dụ `"Unauthorized"` |
| `status` | integer | ✔ | HTTP status code |
| `detail` | string | | Giải thích cụ thể cho lần lỗi này |
