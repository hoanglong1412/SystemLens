# Quyết định thiết kế — UI hiển thị dữ liệu thật (Windows + Linux)

| Mục | Giá trị |
|---|---|
| Ngày | 2026-11-10 |
| Nhánh | `claude/ui-data-display-windows-linux-i9gxh0` |
| Yêu cầu gốc | UI hiện tại (app shell + màn hình Performance › CPU) hiển thị đúng dữ liệu thật; hỗ trợ Windows và Linux; macOS để dạng interface, implement sau; C++ hiện đại; chỉ đọc nhánh hiện tại; AI tự quyết định và ghi lại vào file này |
| Liên quan | [FR-SYS-01, FR-PERF-01, FR-PERF-02](../requirements/functional-requirements.md), [LLD-04](../design/lld/LLD-04-metrics-collection.md), [LLD-05](../design/lld/LLD-05-live-performance-ui.md), [`api/openapi.yaml`](../../api/openapi.yaml) |

User yêu cầu AI **tự quyết định mọi thứ, không hỏi lại**. File này liệt kê từng quyết định, lý do và phương án đã
loại, để user review sau. Mỗi quyết định có ID `D-NN`.

## 1. Phạm vi

| ID | Quyết định | Lý do |
|---|---|---|
| D-01 | "UI hiện tại" = app shell (header, quick stats, tab, status bar) + toàn bộ màn hình Performance › CPU (sidebar resource, gauges, biểu đồ, top processes, summary, specifications, CPU time breakdown, scheduler). Mọi giá trị mock đều được thay bằng dữ liệu thật. | Đó là toàn bộ phần đang hiển thị số liệu cố định (`shellMock.js`, `cpuPerformanceMock.js`). |
| D-02 | Không thêm hành vi mới cho UI: các tab khác, sub-tab "Per-core", nút "Export snapshot", đổi theme, "Desktop Monitor", chọn resource khác CPU vẫn chỉ là hình vẽ như trước. | Yêu cầu chỉ là "hiển thị đúng data", không phải thêm tính năng. |
| D-03 | Màn hình Hello (sample) và nút toggle giữ nguyên. | Không thuộc yêu cầu; vẫn là ví dụ tham chiếu FE ↔ BE. |
| D-04 | Xoá 2 file mock; text tĩnh (tooltip, tiêu đề) chuyển sang file `*Texts.js`. | Tránh để lại số liệu giả có thể bị hiển thị nhầm. |

## 2. Kiến trúc backend

| ID | Quyết định | Lý do / phương án đã loại |
|---|---|---|
| D-05 | Thêm tầng **Provider** (đã có trong quy ước layering của `CLAUDE.md` nhưng chưa có code): 7 interface trong `backend/src/providers/` — `ISystemInfoProvider`, `ICpuProvider`, `IMemoryProvider`, `IDiskProvider`, `INetworkProvider`, `IGpuProvider`, `IProcessProvider`. | Mỗi interface một trách nhiệm; service chỉ phụ thuộc interface nên không có `#ifdef` hệ điều hành ngoài thư mục provider. |
| D-06 | Mỗi hệ điều hành một thư mục: `providers/windows/`, `providers/linux/`, `providers/macos/`. CMake chọn thư mục theo nền tảng; hàm `createPlatformProviders()` (khai báo chung, định nghĩa trong từng thư mục) tạo bộ provider. | Thêm hệ điều hành = thêm thư mục, không sửa code chung. Loại: `#ifdef` rải trong từng hàm (khó đọc, khó test). |
| D-07 | **macOS**: `providers/macos/MacosProviders.cpp` có đủ 7 class implement interface, mọi hàm ném `NotImplementedError`. Backend vẫn build/chạy (API sample hoạt động); 3 API mới trả `500` với thông báo "not implemented on macOS". | Đúng yêu cầu "interface, implement later". Loại: trả số 0 (sẽ hiển thị dữ liệu giả). Phần macOS **chưa được biên dịch thử** (không có máy macOS). |
| D-08 | Provider trả **bộ đếm tích luỹ thô** (CPU time, byte mạng, thời gian disk bận, số context switch…). Việc tính tỉ lệ/tốc độ nằm ở service. | Cùng một công thức cho mọi hệ điều hành; provider đơn giản, dễ thay thế. |
| D-09 | `MetricsCollector` (service) chạy 1 luồng nền `std::jthread`, lấy mẫu **1 giây/lần**, tính delta, giữ lịch sử **61 mẫu (60 giây)** và xuất ra `MetricsSnapshot` bất biến qua interface `IMetricsSource`. | Biểu đồ 60 giây phải liên tục kể cả khi UI chưa mở màn hình; request HTTP chỉ đọc snapshot nên trả về ngay. Loại: tính delta theo từng request (kết quả phụ thuộc tần suất gọi của client). |
| D-10 | 2 service đọc snapshot: `SystemOverviewService`, `PerformanceService`; 2 controller: `SystemController`, `PerformanceController`. | Giữ đúng layering HttpServer → Controller → Service → Provider; controller chỉ đổi DTO sang JSON. |
| D-11 | Lỗi khi lấy mẫu không làm sập backend: collector ghi log 1 lần và lưu lỗi; API trả `500` (Problem) cho tới khi lấy mẫu thành công. | Một sensor hỏng không được làm chết cả ứng dụng. |
| D-12 | Giá trị hệ điều hành không cung cấp được → `null` trong JSON → UI hiển thị `—`. Không bịa số. | "Hiển thị đúng data": thiếu thì báo thiếu. |

## 3. API

| ID | Quyết định | Lý do |
|---|---|---|
| D-13 | 3 endpoint GET, client **poll 1 giây/lần**: `getSystemOverview` (`/api/v1/system/overview`), `getPerformanceResources` (`/api/v1/performance/resources`), `getCpuPerformance` (`/api/v1/performance/cpu`). | Mỗi endpoint ứng với một vùng UI (shell / sidebar / panel CPU). Loại WebSocket: hạ tầng hiện chỉ có HTTP, thêm WebSocket là thay đổi hạ tầng lớn hơn yêu cầu. |
| D-14 | API trả **số thô có đơn vị trong tên field** (`...Percent`, `...Bytes`, `...Mhz`, `...BitsPerSecond`, `...Seconds`); định dạng chuỗi ("2.61 GHz", "612 MB") do frontend làm. | Backend không phụ thuộc cách trình bày. |
| D-15 | Thời điểm dùng chuỗi RFC 3339 UTC (`bootTime`); frontend đổi sang giờ địa phương. | Chuẩn của OpenAPI (`format: date-time`). |
| D-16 | Version hợp đồng API tăng `0.1.0` → `0.2.0`. | Thêm endpoint (không phá vỡ tương thích). |

## 4. Nguồn dữ liệu theo hệ điều hành

| Dữ liệu | Windows | Linux |
|---|---|---|
| Tên máy, OS, user, session | `GetComputerNameExW`, registry `CurrentVersion\ProductName` + `RtlGetVersion`, `GetUserNameW`, `WTSQuerySessionInformationW` | `gethostname`, `/etc/os-release`, `uname`, `getpwuid`, biến `XDG_SESSION_TYPE` |
| Boot time | thời điểm hiện tại − `GetTickCount64` | `btime` trong `/proc/stat` |
| Số process / thread / handle | `GetPerformanceInfo` | số thư mục PID trong `/proc`, `/proc/loadavg`, `/proc/sys/fs/file-nr` |
| Số service | `EnumServicesStatusExW` | không có → `null` |
| CPU time (user/kernel/DPC/interrupt/idle) | `NtQuerySystemInformation(SystemProcessorPerformanceInformation)` | dòng `cpu` trong `/proc/stat` |
| Model, socket, core, cache | registry `CentralProcessor\0`, `GetLogicalProcessorInformationEx` | `/proc/cpuinfo`, `/sys/devices/system/cpu/*/cache` |
| Xung nhịp hiện tại | PDH `\Processor Information(_Total)\% Processor Performance` × base speed | trung bình `scaling_cur_freq`, dự phòng `cpu MHz` trong `/proc/cpuinfo` |
| Nhiệt độ CPU | PDH `\Thermal Zone Information(*)\Temperature` (cao nhất) | `/sys/class/hwmon` (coretemp, k10temp, zenpower, cpu_thermal), dự phòng `thermal_zone` |
| Context switch, interrupt, syscall, DPC | PDH `\System\...`, `\Processor(_Total)\...` (giá trị raw tích luỹ) | `ctxt`, `intr`, `softirq` trong `/proc/stat`; syscall → `null` |
| Bộ nhớ | `GlobalMemoryStatusEx` | `MemTotal − MemAvailable` trong `/proc/meminfo` |
| Disk | `IOCTL_DISK_PERFORMANCE` (`QueryTime − IdleTime`), `IOCTL_STORAGE_QUERY_PROPERTY` (SSD/HDD), ký tự ổ đĩa qua `IOCTL_STORAGE_GET_DEVICE_NUMBER` | `/sys/block/*/stat` (io_ticks), `queue/rotational` |
| Mạng | `GetIfTable2` (Ethernet + Wi-Fi, bỏ filter interface) | `/sys/class/net/*` (bỏ loopback), Wi-Fi nếu có thư mục `wireless` |
| GPU | DXGI (tên adapter) + PDH `\GPU Engine(*)\Utilization Percentage` | `/sys/class/drm/card*` (`gpu_busy_percent` nếu driver có), tên từ `pci.ids` nếu máy có |
| Process | `NtQuerySystemInformation(SystemProcessInformation)` | `/proc/<pid>/stat` |

| ID | Quyết định | Lý do |
|---|---|---|
| D-17 | Windows dùng `NtQuerySystemInformation` cho CPU time và danh sách process. | Một lần gọi lấy đủ CPU time + bộ nhớ của mọi process, kể cả process hệ thống mà `OpenProcess` bị từ chối; cũng phù hợp mục tiêu học OS internals. Offset các field "Reserved" được đặt tên hằng số và chú thích. |
| D-18 | Windows dùng PDH (thư viện có sẵn của Windows) cho các counter không có API trực tiếp. Dùng `PdhAddEnglishCounterW` để không phụ thuộc ngôn ngữ Windows. | Không thêm dependency ngoài. |
| D-19 | Không dùng WMI/COM cho nhiệt độ. | Cần quyền admin và nhiều code COM; PDH Thermal Zone đủ cho đa số máy. Máy không có counter → `—`. |
| D-20 | Ánh xạ khái niệm Windows ↔ Linux: DPC ↔ softirq; Handles ↔ số file handle đang cấp phát; bộ nhớ process: private working set ↔ RSS; `idle` trên Linux gồm cả `iowait` và `steal`. Nhãn UI giữ nguyên theo Figma. | Khái niệm tương đương gần nhất; ghi rõ trong mô tả field của OpenAPI. |
| D-21 | `% CPU` của process = Δ(CPU time) / (Δ(thời gian thực) × số logical processor) — tức chuẩn hoá trên toàn bộ CPU như Task Manager. | Khớp thiết kế (tổng các dòng ≈ % Utilization). |
| D-22 | Gauge "Clock" = xung hiện tại / max(xung tối đa OS báo, xung cao nhất từng thấy). | Windows không báo xung turbo tối đa; tự học theo thời gian để gauge không kẹt ở 100%. |
| D-23 | Sparkline mạng tự co giãn theo đỉnh trong 60 giây (tối thiểu 100 kbit/s). | Giống Task Manager; không có "100%" tự nhiên cho mạng. |
| D-24 | Quick stat "Disk" = disk bận nhất; "Top" = process dùng nhiều bộ nhớ nhất. | Khớp ý nghĩa tooltip trong thiết kế. |
| D-25 | `isSelf` (tô màu dòng trong Top processes) = backend và process cha của nó (Electron). | Thiết kế tô màu "SystemLens.exe". |

## 5. C++

| ID | Quyết định | Lý do |
|---|---|---|
| D-26 | Giữ C++20 (đã cấu hình). Dùng: `std::jthread` + `std::stop_token`, `std::chrono` (duration có kiểu, lịch C++20 để in RFC 3339), `std::optional`, `std::string_view`, `std::span`, `std::from_chars`, `std::filesystem`, `std::ranges`, designated initializers, `enum class`, RAII cho mọi handle của hệ điều hành, `[[nodiscard]]`. | "Code modern". |
| D-27 | Không dùng `std::format`, `std::expected`, module. | Compiler của dự án là Visual Studio 2019: `std::format` chỉ có ở bản 16.10+ với cờ riêng, `std::expected` là C++23. |
| D-28 | Không thêm thư viện bên thứ ba; trên Windows chỉ link thư viện hệ thống (`pdh`, `iphlpapi`, `psapi`, `wtsapi32`, `dxgi`, `ntdll`, `advapi32`). | Quy tắc "không cài thêm công cụ khi chưa hỏi". |
| D-29 | Chưa viết unit test. | Test backend đã bị gỡ tạm ở commit trước để tái cấu trúc; thêm framework test là thêm tooling. Service đã nhận interface qua constructor nên test được bằng provider giả về sau. |

## 6. Frontend

| ID | Quyết định | Lý do |
|---|---|---|
| D-30 | Shell và màn hình CPU đổi từ "render 1 lần" sang **khung tĩnh + `update(data)`**: chỉ thay nội dung các slot động mỗi giây. | Vẽ lại toàn bộ mỗi giây làm tooltip đang mở bị mất và mất focus. |
| D-31 | Tách `*Model.js` (đổi response API → dữ liệu hiển thị, nơi duy nhất định dạng số) khỏi `*View.js` (DOM). | View không biết API; đổi API chỉ sửa model. |
| D-32 | `LiveDataLoop`: gọi 3 API song song, xong mới hẹn lần kế tiếp sau 1 giây (không chồng request). Lỗi → status bar đổi "Disconnected from backend" (chấm đỏ), giữ số liệu cuối, tự thử lại. | Đơn giản, tự hồi phục. |
| D-33 | Trước khi có dữ liệu đầu tiên mọi giá trị hiển thị `—`. | Không hiển thị số giả. |
| D-34 | Màu ô CPU % của Top processes nội suy tuyến tính theo giá trị so với dòng cao nhất (2 màu đầu mút lấy từ thiết kế). | Trước đây màu được tính sẵn trong mock. |
| D-35 | Thanh CPU time breakdown: mỗi phần > 0 rộng tối thiểu 6 px, Idle chiếm phần còn lại. | Thiết kế vẽ các phần rất nhỏ vẫn nhìn thấy được. |
| D-36 | Electron tìm backend theo nền tảng: Windows `backend/build/Debug/systemlens-backend.exe`, khác: `backend/build/systemlens-backend`. | Generator một-cấu-hình (Ninja/Make) trên Linux không có thư mục `Debug`. |

## 7. Công cụ, kiểm chứng và giới hạn

| ID | Nội dung |
|---|---|
| D-37 | Phiên làm việc chạy trong container Linux, **không có PowerShell**: hook `fix-format.ps1` và `Tool\check_*.ps1` không chạy được. AI tự chuẩn hoá UTF-8 không BOM + CRLF bằng script tương đương và kiểm tra lại toàn bộ file đã đổi trước khi commit. |
| D-38 | Để kiểm tra code Windows biên dịch được, AI cài cross-compiler MinGW-w64 **chỉ trong container tạm** (không thêm gì vào repo hay máy của user). MinGW khác MSVC nên đây chỉ là kiểm tra cú pháp/kiểu/link. |
| D-39 | Đã kiểm chứng: **Linux** — build (GCC 13, `-Wall -Wextra -Wpedantic` không cảnh báo), chạy backend thật, gọi 3 API (có và không có token), chạy renderer trong Chromium headless trỏ vào backend thật và chụp ảnh `evi/app_cpu_performance_live_linux.png`; chưa chạy qua Electron (container chưa cài `node_modules`). **Windows** — chỉ biên dịch chéo + link bằng MinGW-w64, **chưa chạy thật, chưa build bằng MSVC**. **macOS** — chưa biên dịch. |
| D-40 | Không thêm script chạy cho Linux trong `Tool/`: quy tắc CRLF làm hỏng shell script. Lệnh build/chạy trên Linux ghi trong `README.md`. |
| D-41 | Không tạo pull request (chưa được yêu cầu). |
| D-42 | Dependency `FetchContent` (cpp-httplib, nlohmann/json) không tải được qua proxy của container nên khi kiểm chứng AI dùng bản clone cùng version qua `FETCHCONTENT_SOURCE_DIR_*`; `CMakeLists.txt` không đổi cách tải. |

## 9. Pha 2 — các màn hình còn lại (2026-11-11)

User yêu cầu: "tự thiết kế và code tiếp cho tất cả màn hình còn lại". AI tự quyết định như pha 1.

| ID | Quyết định | Lý do |
|---|---|---|
| D-43 | "Màn hình còn lại" = 6 tab (Processes, App history, Startup, Users, Details, Services) + trong Performance: panel Memory, Disk, Ethernet / Wi-Fi, GPU và sub-tab CPU "Per-core". Không làm: "Export snapshot", đổi theme, "Desktop Monitor", nút toggle Hello (giữ nguyên như D-03). | Đó là mọi phần còn lại hiển thị dữ liệu; 3 nút kia là hành động, không phải màn hình. |
| D-44 | Bấm tab trên thanh tab → chuyển màn hình. Mỗi màn hình chỉ poll API khi đang hiển thị (dừng khi bị ẩn); shell luôn poll overview mỗi giây. | Tránh gọi API nặng (danh sách process) khi người dùng không xem. |
| D-45 | Chỉ đọc: không kết thúc process, không bật/tắt startup item, không start/stop service. | Hành động phá huỷ / cần quyền admin, ngoài yêu cầu "hiển thị". |
| D-46 | **Processes** = gom theo tên process (dòng nhóm bung/thu được, tổng CPU + bộ nhớ). **Details** = bảng phẳng từng process, nhiều cột hơn (PID, tên, trạng thái, user, CPU %, CPU time, bộ nhớ, thread, PID cha, command line), sắp xếp theo cột và lọc theo chữ. | Hai tab có hai cách xem khác nhau, giống Task Manager. |
| D-47 | **App history** = tích luỹ trong RAM kể từ lúc backend khởi động (CPU time, bộ nhớ đỉnh, số instance, bộ nhớ hiện tại theo tên process). Không lưu ra đĩa, không có cột mạng. | Không có nguồn dữ liệu lịch sử chung cho Windows và Linux; lưu đĩa là thêm tính năng. |
| D-48 | Trạng thái process: Linux lấy từ `/proc/<pid>/stat`; Windows không có → `null` → `—`. Command line: chỉ Linux; Windows `null` (cần đọc PEB của process khác). | Không bịa dữ liệu (D-12). |
| D-49 | **Services**: Windows dùng Service Control Manager (tên, display name, trạng thái, kiểu khởi động, PID); Linux gọi `systemctl` (tham số cố định, qua `popen`) để lấy trạng thái + kiểu khởi động (không có PID). Máy không dùng systemd → `available=false` kèm lý do, UI hiện banner. | Không có API systemd không phụ thuộc thư viện ngoài. |
| D-50 | **Startup**: Windows đọc các khoá registry `...\\Run` (HKCU, HKLM, WOW6432Node) và thư mục Startup, trạng thái bật/tắt theo `StartupApproved`; Linux đọc file `.desktop` trong `~/.config/autostart` và `/etc/xdg/autostart` (bản của user che bản hệ thống cùng tên). | Chính là nơi Task Manager đọc. |
| D-51 | **Users**: phiên đăng nhập từ WTS (Windows) / utmp (Linux); utmp rỗng (container, server không GUI) → thêm một dòng cho user đang chạy backend. Bảng "mức dùng theo user" gom từ danh sách process theo chủ sở hữu (Windows: process không đọc được token → nhóm `(unknown)`). | Dữ liệu thật; dòng dự phòng ghi rõ trong docs. |
| D-52 | Endpoint inventory (services, startup, users) trả `200` với `available=false` + `reason` khi nền tảng không hỗ trợ (macOS, không có systemd), thay vì `500`. | Đây là trạng thái bình thường, không phải lỗi hệ thống. |
| D-53 | Tần suất poll: panel Performance 1 s; Processes / Details / App history 2 s (payload lớn); Services / Startup / Users 5 s. | Cân bằng độ mới và tải. |
| D-54 | Panel Memory / Disk / Network / GPU dùng chung một view "resource detail" (tiêu đề, biểu đồ, thẻ thống kê, bảng tuỳ chọn) cấu hình bằng model riêng cho từng loại. | Một view, bốn model: ít code, cùng giao diện. |
| D-55 | Thêm 3 provider interface: `ISystemServiceProvider` (không đặt `IServiceProvider` vì trùng kiểu COM của Windows), `IStartupProvider`, `IUserSessionProvider`; mở rộng struct của provider CPU / memory / disk / network / GPU / process. Các provider này không giữ trạng thái và được gọi trực tiếp từ luồng request (không qua sampler), trừ process/CPU/disk/network/GPU vẫn qua `MetricsCollector`. | Danh sách service/startup/user thay đổi chậm, không cần lịch sử. |
| D-56 | Memory: thêm available, cached, committed, commit limit (cả hai hệ điều hành); không tách swap / page file. Disk: model, dung lượng, tốc độ đọc / ghi. Network: mô tả adapter, MAC, tốc độ liên kết. GPU: tổng bộ nhớ, bộ nhớ đã dùng (chỉ Linux amdgpu). | Phần tương đương chung của hai hệ điều hành. |
| D-57 | Sắp xếp và lọc làm ở frontend; backend trả nguyên danh sách. | Đơn giản, danh sách chỉ vài trăm dòng. |
| D-58 | Biểu đồ đọc/ghi của Disk và gửi/nhận của Network gửi **số thô** theo đơn vị thật (bytes/s, bit/s); frontend tự co giãn theo đỉnh trong 60 s. | Đúng D-14 và D-23. |
| D-59 | Không thêm tooling; Windows vẫn chỉ biên dịch chéo bằng MinGW-w64 (D-38/D-39). | Như pha 1. |
| D-60 | Màn hình mặc định khi mở app là tab **Performance** (không còn là màn hình Hello); Hello vẫn truy cập được bằng nút tạm `⇄ Hello` (đổi thành `⇄ Back` khi đang ở Hello). | Có thanh tab thật thì tab active và màn hình hiển thị phải khớp nhau. Sửa lại D-03. |
| D-61 | Bấm cột số (căn phải) lần đầu → sắp xếp giảm dần; bấm lại → đảo chiều. Cột chữ: tăng dần trước. | Người dùng thường muốn xem giá trị lớn nhất trước. |
| D-62 | Sidebar Performance bấm được: chọn resource → đổi panel ngay và tải dữ liệu của resource đó ngay (không chờ timer); chỉ tải dữ liệu của resource đang chọn cùng danh sách sidebar. | Giảm tải backend / JSON; phản hồi tức thì. |
| D-63 | Biểu đồ của panel mới (Memory / Disk / Network / GPU / Per-core) co giãn theo chiều rộng của vùng chứa (plot "fluid", nét vẽ giữ độ dày); các card của CPU Overview vẫn kích thước cố định theo Figma. | Panel mới không có thiết kế Figma; layout linh hoạt hơn. |
| D-64 | Không có thiết kế Figma cho các màn hình mới: AI tự thiết kế theo token màu, card, bảng và font hiện có (xem ảnh trong `evi/`). | Không đọc được Figma ở pha này (không cần: giữ phong cách có sẵn). |

## 8. Việc để lại

- Implement provider macOS (nay gồm cả 3 provider mới ở pha 2).
- Chạy thử trên Windows thật (đặc biệt: PDH counter, GPU, disk IOCTL) và build bằng MSVC 2019.
- Nhiệt độ GPU trên Windows; số service trên Linux; syscall/giây trên Linux.
- Unit test cho `MetricsCollector` và 2 service với provider giả.
