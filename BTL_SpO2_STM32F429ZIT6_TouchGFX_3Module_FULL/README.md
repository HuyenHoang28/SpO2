# BTL SpO2 — STM32F429ZIT6 + MAX30102 + Tiny RTC + TouchGFX

Bài tập lớn: thiết bị đo **nhịp tim (BPM)** và **nồng độ oxy trong máu (SpO2)** dùng
kit **STM32F429ZIT6** hiển thị kết quả trên **LCD/TFT** qua **TouchGFX**, kèm mốc
thời gian đo lấy từ module **Tiny RTC (DS3231/DS1307)**.

> Không dùng OLED, không dùng LED rời, không dùng ESP32/Blynk. Tất cả UI nằm trên
> LCD của kit STM32F429ZIT6 và được vẽ bằng TouchGFX.

## 1. Hệ thống 3 module

| Module        | Nhiệm vụ                                  | Giao tiếp     | Địa chỉ I2C |
|---------------|-------------------------------------------|---------------|-------------|
| STM32F429ZIT6 | MCU chính, chạy TouchGFX + xử lý tín hiệu | —             | —           |
| MAX30102      | Cảm biến quang đo Red/IR → BPM & SpO2     | I2C1 PB8/PB9  | `0x57`      |
| Tiny RTC      | Đồng hồ thời gian thực (giờ, ngày)        | I2C1 PB8/PB9  | `0x68`      |

Đấu nối chung một bus I2C1:

| STM32F429ZIT6   | MAX30102 | Tiny RTC |
|-----------------|----------|----------|
| 3.3V            | VCC      | VCC      |
| GND             | GND      | GND      |
| PB8 / I2C1_SCL  | SCL      | SCL      |
| PB9 / I2C1_SDA  | SDA      | SDA      |

Xem sơ đồ trong [`images/block_diagram.png`](images/block_diagram.png) và
[`images/i2c_wiring.png`](images/i2c_wiring.png).

## 2. Cấu trúc thư mục

```
BTL_SpO2_STM32F429ZIT6_TouchGFX_3Module_FULL/
├── README.md                                           <- (file này) tổng quan project
├── Bao_cao_BTL_SpO2_...docx                            <- Báo cáo BTL
├── Huong_dan_lam_Project_SpO2_...docx                  <- Hướng dẫn từng bước làm project
├── STM32F429ZIT6_SpO2_TouchGFX_3Module_CODE.zip        <- Bản nén của thư mục code
│
├── code/                                               <- Source code chính (dạng thư mục)
│   ├── README_HUONG_DAN.md                             <- Hướng dẫn tích hợp code
│   ├── Config/
│   │   └── STM32CubeMX_Config.md                       <- Cấu hình CubeMX (clock, I2C, TouchGFX, LCD)
│   ├── Core/
│   │   ├── Inc/                                        <- Header driver & thuật toán
│   │   │   ├── health_monitor.h                        <- API HealthMonitor_Init/Process/GetData
│   │   │   ├── max30102.h                              <- Driver MAX30102
│   │   │   ├── spo2_algorithm.h                        <- Thuật toán BPM/SpO2 từ Red/IR
│   │   │   └── tiny_rtc.h                              <- Driver DS3231/DS1307
│   │   └── Src/                                        <- Implementation .c
│   │       ├── health_monitor.c
│   │       ├── main_integration_example.c              <- Mẫu tích hợp vào main.c
│   │       ├── max30102.c
│   │       ├── spo2_algorithm.c
│   │       └── tiny_rtc.c
│   └── TouchGFX/
│       ├── README_TouchGFX_UI.md                       <- Hướng dẫn tạo Screen1 trong Designer
│       └── gui/
│           ├── include/gui/
│           │   ├── model/{Model,ModelListener}.hpp
│           │   └── screen1_screen/{Screen1Presenter,Screen1View}.hpp
│           └── src/
│               ├── model/Model.cpp
│               └── screen1_screen/{Screen1Presenter,Screen1View}.cpp
│
├── STM32F429ZIT6_SpO2_TouchGFX_3Module_CODE/           <- Bản giải nén tương đương của code/
│   └── ... (giống hệt code/)                              (giữ song song để tiện copy vào CubeIDE)
│
└── images/                                             <- Ảnh minh hoạ cho báo cáo
    ├── block_diagram.png                               <- Sơ đồ khối hệ thống
    ├── i2c_wiring.png                                  <- Sơ đồ đấu dây I2C
    ├── software_flow.png                               <- Luồng phần mềm (ISR → Algorithm → UI)
    └── touchgfx_mockup.png                             <- Mockup giao diện TouchGFX
```

> Hai thư mục `code/` và `STM32F429ZIT6_SpO2_TouchGFX_3Module_CODE/` có nội dung
> **giống nhau**. Bản `_CODE/` (và file `.zip` tương ứng) là bản đóng gói tiện
> mang đi copy vào project STM32CubeIDE; `code/` là bản làm việc.

## 3. Luồng phần mềm

```
MAX30102 --I2C--> STM32 ISR/Poll ──► SpO2Algorithm_AddSample(red, ir)
                                    │
                                    ▼
                          SpO2Algorithm_GetResult() ── BPM, SpO2, valid
                                    │
Tiny RTC --I2C--> TinyRTC_GetTime() │
                                    ▼
                      HealthMonitor_Process10ms()  (gọi mỗi 10ms)
                                    │
                                    ▼
                      HealthMonitor_GetData() ──► Model::tick()
                                                  │
                                                  ▼
                                    Screen1Presenter ──► Screen1View
                                                          │
                                                          ▼
                                                LCD TouchGFX (TextArea)
```

Xem ảnh chi tiết ở [`images/software_flow.png`](images/software_flow.png).

## 4. Bắt đầu nhanh

1. Cài **STM32CubeIDE** + **TouchGFX Designer** (đi kèm CubeIDE là đủ).
2. Tạo project mới cho **STM32F429ZIT6** (khuyến nghị: Board Selector nếu dùng
   STM32F429I-DISC1 để CubeMX tự cấu hình LCD/SDRAM/LTDC/DMA2D).
3. Cấu hình theo [`code/Config/STM32CubeMX_Config.md`](code/Config/STM32CubeMX_Config.md):
   - Bật **Graphics → TouchGFX**.
   - Bật **CRC, DMA2D, LTDC, FMC/SDRAM** cho LCD.
   - Bật **I2C1**: `PB8=SCL`, `PB9=SDA`, tốc độ 100 kHz khi test, có thể lên 400 kHz sau.
   - (Tuỳ chọn) Bật **USART3 115200** để in log Red/IR/BPM/SpO2 khi debug.
4. **Generate code** với toolchain STM32CubeIDE.
5. Copy source:
   - `code/Core/Inc/*.h`  → `Core/Inc/` của project.
   - `code/Core/Src/*.c`  → `Core/Src/` của project *(trừ `main_integration_example.c` — chỉ tham khảo)*.
   - Trong TouchGFX Designer, tạo `Screen1` với các TextArea theo
     [`code/TouchGFX/README_TouchGFX_UI.md`](code/TouchGFX/README_TouchGFX_UI.md)
     (`textSpo2`, `textBpm`, `textTime`, `textDate`, `textStatus`), rồi copy các
     file trong `code/TouchGFX/gui/` đè lên `TouchGFX/gui/` tương ứng.
6. Trong `main.c`:
   ```c
   HealthMonitor_Init(&hi2c1);
   // gọi mỗi 10ms (SysTick / TIM / RTOS task)
   HealthMonitor_Process10ms();
   ```
7. Build → nạp → đặt ngón tay lên MAX30102 → xem SpO2/BPM/giờ hiện lên LCD.

## 5. Các trạng thái hệ thống

Enum `HealthStatus_t` trong `health_monitor.h`:

- `HM_STATUS_INIT` — đang khởi tạo.
- `HM_STATUS_PLACE_FINGER` — chưa phát hiện ngón tay, nhắc người dùng đặt lên.
- `HM_STATUS_MEASURING` — đang lấy đủ mẫu để tính.
- `HM_STATUS_VALID` — kết quả hợp lệ (Normal).
- `HM_STATUS_LOW_SPO2` — SpO2 dưới ngưỡng cảnh báo.
- `HM_STATUS_SENSOR_ERROR` — không giao tiếp được MAX30102.
- `HM_STATUS_RTC_ERROR` — không giao tiếp được Tiny RTC.

Hàm `HealthMonitor_StatusText()` trả về chuỗi hiển thị lên `textStatus`.

## 6. Lưu ý quan trọng

- Nếu Tiny RTC là **DS1307 loại 5V**, **không** để SDA/SCL bị kéo lên 5V — STM32
  chỉ 3.3V, dễ hỏng chân MCU. Ưu tiên module DS3231 3.3V.
- Nếu sensor không nhận: dùng **I2C scanner** hoặc đọc `PART_ID` MAX30102 (phải
  trả về `0x15`).
- **Công thức SpO2 trong `spo2_algorithm.c` là công thức demo/học tập, không có
  giá trị y tế** — không dùng để chẩn đoán hay thay thế thiết bị y tế đã kiểm
  chuẩn.

## 7. Tài liệu đi kèm

- `Bao_cao_BTL_SpO2_STM32F429ZIT6_TouchGFX_3Module.docx` — báo cáo BTL (nộp).
- `Huong_dan_lam_Project_SpO2_STM32F429ZIT6_TouchGFX_3Module.docx` — hướng dẫn
  làm project chi tiết từng bước (dùng để làm theo).
- `code/README_HUONG_DAN.md` — README nội bộ của phần code.
- `code/Config/STM32CubeMX_Config.md` — cấu hình CubeMX.
- `code/TouchGFX/README_TouchGFX_UI.md` — thiết kế màn hình TouchGFX.
