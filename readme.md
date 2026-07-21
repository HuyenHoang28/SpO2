# SpO2 STM32F429I-DISCO — stable display build v9

This build fixes the uniform blue/cyan LCD symptom by restoring the official
TouchGFX event loop and by removing the incorrect use of PD12/PD13 as status
LEDs. See `docs/TOUCHGFX_BLUE_SCREEN_ROOT_CAUSE_V9.md`.

**First hardware test:** disconnect MAX30102 and Tiny RTC, flash v9, reset the
board, and verify Screen1. Reconnect the modules only after the GUI is visible.

# PROJECT ĐO SpO₂ – STM32F429ZIT6 + MAX30102 + TINY RTC + TOUCHGFX

Project hoàn chỉnh cho kit **STM32F429I-DISC1/STM32F429ZIT6**, cảm biến
**MAX30102** và module **Tiny RTC DS1307 hoặc DS3231**. LCD trên kit hiển thị
BPM, SpO₂, thời gian, ngày và hai đồ thị lịch sử.

> Đây là project học tập, không phải thiết bị y tế và không dùng để chẩn đoán.

## 1. Chức năng

- Đọc FIFO Red/IR của MAX30102 bằng STM32 HAL I²C.
- Phát hiện có/không có ngón tay.
- Tính nhịp tim và SpO₂ từ cửa sổ 100 mẫu.
- Tính lại kết quả sau mỗi 25 mẫu mới.
- Đọc ngày giờ từ Tiny RTC.
- Tự thử kết nối lại khi cảm biến hoặc RTC bị ngắt.
- Màn 1: BPM, SpO₂, giờ, ngày và trạng thái đo.
- Màn 2: đồ thị lịch sử BPM và SpO₂.
- Chuyển màn bằng cách chạm LCD hoặc nhấn nút USER PA0.
- TouchGFX Simulator có dữ liệu mô phỏng để kiểm tra giao diện không cần phần cứng.

## 2. Đấu dây

Dùng **I2C3 trên PA8/PC9** và chia sẻ bus với bộ điều khiển cảm ứng STMPE811. Không dùng PB6 vì PB6 là `FMC_SDNE1` của SDRAM; không dùng PB8/PB9 vì hai chân này điều khiển màu xanh của LCD (`LTDC_B6`, `LTDC_B7`).

| STM32F429I-DISC1 | MAX30102 | Tiny RTC |
|---|---|---|
| PA8 – I2C3_SCL | SCL | SCL |
| PC9 – I2C3_SDA | SDA | SDA |
| 3V | VIN/VCC | DS3231 VCC |
| GND | GND | GND |

Hai module dùng địa chỉ khác nhau nên có thể dùng chung bus. Nếu module Tiny RTC dùng
**DS1307**, IC này thường cần nguồn 5 V; phải dùng mạch chuyển mức I²C hoặc xử lý
điện trở kéo lên đúng cách để không đưa 5 V vào bus của MAX30102. DS3231 phù hợp
hơn với bus 3,3 V.

Địa chỉ sử dụng:

- MAX30102: `0x57`.
- Tiny RTC: `0x68`.

Chi tiết: [`docs/WIRING.md`](docs/WIRING.md).

## 3. Chọn loại RTC

Trong `Core/Src/main.c`:

```c
#define SPO2_RTC_KIND TINY_RTC_DS1307
```

- Giữ `TINY_RTC_DS1307` nếu module ghi **Tiny RTC / DS1307**.
- Đổi thành `TINY_RTC_DS3231` nếu IC trên module là **DS3231**.

Khi pin RTC mới lắp và giờ chưa hợp lệ, đặt `SPO2_SET_RTC_ON_BOOT` thành `1`,
chỉnh ngày giờ trong `initial_time`, nạp chương trình một lần, sau đó trả macro
về `0` và build lại.

## 4. Mở và build bằng STM32CubeIDE

1. Mở STM32CubeIDE.
2. Chọn **File → Import → Existing Projects into Workspace**.
3. Chọn thư mục `SpO_2/STM32CubeIDE`.
4. Chọn project `STM32F429I_DISCO_REV_D01`.
5. Chọn **Project → Clean**, sau đó **Build Project**.
6. Kết nối ST-LINK của kit và chọn **Run** hoặc **Debug**.

Các file driver mới đã được thêm vào linked resources của project CubeIDE:

```text
Core/Inc/max30102.h
Core/Inc/spo2_algorithm.h
Core/Inc/spo2_app.h
Core/Inc/tiny_rtc.h
Core/Inc/finger_detector.h
Core/Src/max30102.c
Core/Src/spo2_algorithm.c
Core/Src/spo2_app.c
Core/Src/tiny_rtc.c
Core/Src/finger_detector.c
```

Hướng dẫn chi tiết: [`docs/BUILD_AND_FLASH.md`](docs/BUILD_AND_FLASH.md).

## 5. Mở TouchGFX Designer

Mở file:

```text
TouchGFX/SpO_2.touchgfx
```

Project đã được đồng bộ hoàn toàn về framebuffer **RGB565 16-bit**. Không đổi
riêng một phần sang 32-bit vì sẽ làm `LCD`, `LTDC`, Canvas Painter và framebuffer
không cùng định dạng.

## 6. Luồng chạy

```text
MAX30102 + Tiny RTC
        │ I2C3 PA8/PC9
        ▼
defaultTask, mỗi 20 ms
        │
        ├─ đọc FIFO MAX30102
        ├─ cập nhật buffer 100 mẫu
        ├─ tính BPM/SpO₂ mỗi 25 mẫu
        └─ đọc RTC mỗi 1 giây
        │
        ▼
SpO2AppSnapshot
        │
        ▼
TouchGFX Model → Presenter → Screen1/Screen2
```

Việc đọc cảm biến không đặt trong `Model::tick()`, tránh I²C chặn GUI khi thiết
bị chưa cắm hoặc mất kết nối.

## 7. Cấu hình đo mặc định

- ADC MAX30102: 100 mẫu/giây.
- FIFO average: 4 mẫu, tương đương khoảng 25 mẫu FIFO/giây.
- Pulse width: 411 µs, ADC 18-bit.
- Dòng LED Red/IR: khoảng 6,2 mA.
- Cửa sổ phân tích: 100 mẫu, khoảng 4 giây.
- Chu kỳ cập nhật kết quả: 25 mẫu, khoảng 1 giây.
- Ngưỡng phát hiện ngón tay: `50000` IR.

Nếu tín hiệu trên module thực tế quá yếu, tăng dòng LED từng bước nhỏ trong
`MAX30102_InitDefault()`; không tăng thẳng lên mức tối đa.

## 8. Kiểm tra đã thực hiện

- Test thuật toán bằng tín hiệu PPG mô phỏng: khoảng `75 BPM`, `97% SpO₂`.
- Kiểm tra cú pháp toàn bộ driver C bằng GCC.
- Kiểm tra cú pháp lớp Model/Presenter/View ở cả chế độ target và Simulator.
- Kiểm tra đồng nhất LCD/LTDC/DMA2D/Canvas ở RGB565 16-bit.
- Kiểm tra JSON TouchGFX, XML text database và project linked resources.

Danh sách lỗi đã sửa: [`docs/FIXES_APPLIED.md`](docs/FIXES_APPLIED.md).

## 9. Bản sửa hiển thị TouchGFX v8

Bản v8 dùng một framebuffer RGB565 duy nhất tại `0xD0000000`, software painter
`NoDMA` và vòng GUI không phụ thuộc ngắt LTDC/DMA2D. Linker sẽ báo lỗi nếu
framebuffer không nằm đúng địa chỉ hoặc không đúng 153600 byte.

Sau khoảng một giây, firmware có thể chuyển sang màu chẩn đoán nếu khung đầu tiên
không hợp lệ: cam là GUI task chưa chạy, vàng là GUI chạy nhưng chưa vẽ widget,
đỏ là lỗi khởi tạo và tím là HardFault. Chi tiết nằm trong:

- `docs/TOUCHGFX_DEEP_REVIEW_V8.md`
- `docs/TOUCHGFX_DISPLAY_FIX_V8.md`

## v10 sensor and RTC update

See `docs/SENSOR_RTC_FIX_V10.md`. This revision adds I2C bus recovery,
MAX30102 retry/stronger LED drive/more sensitive finger detection, automatic RTC
initialization from the CubeIDE build timestamp, and a live software-clock
fallback when the RTC module is absent.


## v11 MAX30102 stability update

See `docs/MAX30102_STABILITY_FIX_V11.md`. This revision fixes the case where
`PLACE FINGER` changed to `MAX30102 MISSING` after a transient FIFO/I2C error.
The device must now fail repeated address probes before it is classified as
missing. RTC probing is delayed so an unstable RTC cannot interrupt the sensor.

## v14 MAX30102 measurement fix

The finger detector was rewritten. The status line shows raw IR while waiting and
buffer progress while measuring. See `docs/MAX30102_ALGORITHM_REVIEW_V14.md`.


## v14.1 CubeIDE link fix

`Core/Src/finger_detector.c` is now explicitly linked into the STM32CubeIDE
project as `Application/User/finger_detector.c`. This fixes the linker errors
for `FingerDetector_Reset`, `FingerDetector_Update`, and the getter functions.
