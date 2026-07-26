# BÀI TẬP LỚN - ĐO NHỊP TIM VÀ SpO₂

Hệ thống đo nhịp tim (BPM) và nồng độ oxy trong máu (SpO₂) sử dụng STM32F429I-DISCO, cảm biến MAX30102, module Tiny RTC và giao diện TouchGFX.

## GIỚI THIỆU

**Đề bài/Mục tiêu sản phẩm:**
Thiết kế thiết bị đo nhịp tim và SpO₂ chạy trực tiếp trên kit STM32F429I-DISCO (MCU STM32F429ZIT6), dùng phương pháp quang dung tích (PPG) qua cảm biến MAX30102, hiển thị số liệu tức thời và đồ thị lịch sử trên LCD tích hợp của kit thông qua TouchGFX.

**Hướng tiếp cận:**
MAX30102 và Tiny RTC được nối chung một bus I²C (I2C3, PA8/PC9) với bộ điều khiển cảm ứng STMPE811 của kit. Một task FreeRTOS (`defaultTask`) đọc FIFO Red/IR của cảm biến mỗi 20 ms, đưa qua một bộ dò ngón tay riêng (`finger_detector.c`) rồi tích luỹ vào vòng đệm 100 mẫu. Cứ mỗi 25 mẫu mới, thuật toán `spo2_algorithm.c` tính lại BPM (tự tương quan) và SpO₂ (ratio-of-ratios) như hai giá trị hợp lệ độc lập. Kết quả được đóng gói vào một struct `SpO2AppSnapshot` dùng chung, TouchGFX `Model::tick()` đọc snapshot này và đẩy sang hai màn hình `Screen1View`/`Screen2View`. Việc đọc cảm biến được tách hẳn khỏi vòng vẽ GUI để một lỗi I²C không làm treo giao diện.

**Sản phẩm:**

1. Đo BPM và SpO₂ từ tín hiệu PPG Red/IR của MAX30102, hai giá trị được xác nhận hợp lệ độc lập.
2. Bộ dò ngón tay thích nghi theo baseline học được, không dùng ngưỡng cố định.
3. Tự động chỉnh dòng LED (trong khoảng `0x20`–`0x7F`) khi tín hiệu quá yếu hoặc ADC bão hoà.
4. Tự chẩn đoán và force-restart cấu hình đo khi FIFO của cảm biến ngừng sinh dữ liệu.
5. Hiển thị giờ/ngày từ Tiny RTC (DS1307/DS3231), có đồng hồ phần mềm dự phòng khi mất RTC.
6. Hai màn hình TouchGFX: Screen1 hiển thị số liệu tức thời + trạng thái đo, Screen2 hiển thị đồ thị lịch sử BPM/SpO₂ (giữ nguyên 160 mẫu khi chuyển qua lại giữa hai màn hình).
7. Chuyển màn hình bằng chạm LCD hoặc nút USER (PA0).

> Đây là project học tập/mô phỏng kỹ thuật, không phải thiết bị y tế và không dùng để chẩn đoán.

---

## TÁC GIẢ

- **Môn học:** Hệ thống nhúng
- **Giảng viên hướng dẫn:** Thầy Nguyễn Đức Tiến
- **Nhóm:** Hungyenian

**Phân chia công việc**

| Thành viên | Nhiệm vụ |
| --- | --- |
| Hoàng Trịnh Trường Phúc | Nhúng: HW & driver ngoại vi|
| Trần Hữu Đạt | Truyền thông UART & tài liệu|
| Hoàng Thị Hà Huyền | Nhúng: GUI TouchGFX & tích hợp |
| Nguyễn Trương Ngọc Mai | Xử lý tín hiệu & thuật toán |




## MÔI TRƯỜNG HOẠT ĐỘNG

Hệ thống hoạt động trên bo STM32F429I-DISCO, dùng màn hình LCD-TFT tích hợp; MAX30102 và Tiny RTC được nối trực tiếp vào vi điều khiển qua I²C.

**Bill of materials**

| STT | Linh kiện | Số lượng | Vai trò |
| ---: | --- | ---: | --- |
| 1 | STM32F429I-DISCO (STM32F429ZIT6) | 1 | Xử lý trung tâm và hiển thị TouchGFX |
| 2 | Cảm biến MAX30102 | 1 | Đo PPG Red/IR |
| 3 | Module Tiny RTC (DS1307 hoặc DS3231) | 1 | Cung cấp thời gian thực |
| 4 | Dây jumper | Theo nhu cầu | Nối I²C và nguồn |
| 5 | Cáp USB (ST-Link tích hợp trên kit) | 1 | Nạp code, debug, cấp nguồn |

**Công cụ phát triển**

| Công cụ/thư viện | Phiên bản hoặc cấu hình |
| --- | --- |
| STM32CubeIDE | Build trực tiếp từ mã nguồn có sẵn, **không** Generate Code lại từ `.ioc` |
| TouchGFX | Framebuffer RGB565 16-bit, target `STM32F429I_DISCO_REV_D01` |
| FreeRTOS | CMSIS-RTOS v2, task `defaultTask` chu kỳ 20 ms |
| Màn hình | 240 × 320, RGB565, LTDC + DMA2D + FMC/SDRAM |

---

## SƠ ĐỒ SCHEMATIC

### Kết nối tín hiệu

| STM32F429I-DISCO | Module ngoại vi | Chức năng |
| --- | --- | --- |
| PA8 – I2C3_SCL | MAX30102 SCL, Tiny RTC SCL | Bus I²C dùng chung với STMPE811 (cảm ứng) |
| PC9 – I2C3_SDA | MAX30102 SDA, Tiny RTC SDA | Bus I²C dùng chung với STMPE811 |
| 3V3 | MAX30102 VIN/VCC, Tiny RTC VCC | Cấp nguồn (ưu tiên module DS3231, hoạt động tốt ở 3,3 V) |
| GND | MAX30102 GND, Tiny RTC GND | Nối chung mass toàn hệ thống |
| PA0 | Nút USER trên bo | Chuyển đổi Screen1 ↔ Screen2 |

```text
                         +----------------------+
MAX30102 SCL/SDA ------->|                      |
Tiny RTC SCL/SDA ------->|   STM32F429I-DISCO   |----> LCD TouchGFX
     (I2C3, PA8/PC9)     |   (STM32F429ZIT6)    |      (Screen1/Screen2)
Nút USER (PA0) --------->|                      |
                         +----------------------+
```

**Không dùng I2C1 mặc định trên PB6/PB8/PB9**: PB6 là `FMC_SDNE1` (chip-select SDRAM), PB8/PB9 là `LTDC_B6`/`LTDC_B7` (kênh màu xanh của LCD). Dùng nhầm sẽ hỏng SDRAM hoặc sai màu hiển thị — đây là lý do project chọn I2C3 trên PA8/PC9 thay vì I2C1 mặc định. Chi tiết: [`docs/WIRING.md`](../docs/WIRING.md), [`DO_NOT_REGENERATE_FROM_IOC.txt`](../DO_NOT_REGENERATE_FROM_IOC.txt).

Địa chỉ I²C: MAX30102 = `0x57`, Tiny RTC = `0x68` — khác nhau nên dùng chung một bus không xung đột. Nếu module Tiny RTC là DS1307 (thường thiết kế 5 V), cần xử lý đúng mức điện áp bus để không đưa 5 V vào chân I²C 3,3 V của MAX30102.

---

## TÍCH HỢP HỆ THỐNG

### Luồng hoạt động

1. `defaultTask` khởi động, khoá bus I²C, phục hồi bus (`I2C3_BusRecover`) rồi gọi `SpO2App_Init()`.
2. Mỗi 20 ms, `SpO2App_Process()` đọc tối đa 8 mẫu FIFO Red/IR mới từ MAX30102.
3. Mỗi mẫu được đưa qua `FingerDetector_Update()` để cập nhật trạng thái có/không có ngón tay, đồng thời luôn được đẩy vào vòng đệm 100 mẫu bất kể bộ dò có nhận ra tay hay không.
4. Cứ mỗi ~2 giây, dòng LED được tự chỉnh (`auto_adjust_led_current`) nếu tín hiệu bão hoà hoặc quá yếu.
5. Khi đủ 100 mẫu và có 25 mẫu mới, `SpO2Algorithm_Compute()` tính lại BPM (tự tương quan, dự phòng đếm đỉnh) và SpO₂ (ratio-of-ratios), mỗi giá trị được xác nhận hợp lệ độc lập.
6. Nếu FIFO ngừng sinh mẫu trên 1,5 giây dù cảm biến vẫn trả lời trên bus, `MAX30102_ForceMeasurement()` được gọi để nạp lại cấu hình đo tối thiểu.
7. Tiny RTC được đọc mỗi giây (hoặc dùng đồng hồ phần mềm nếu RTC vắng mặt/lỗi).
8. Kết quả được gói vào `SpO2AppSnapshot`; TouchGFX `Model::tick()` phát hiện snapshot mới qua số thứ tự `sequence`, cập nhật lịch sử đồ thị rồi gọi `Screen1View`/`Screen2View` để vẽ lại.
9. Chạm màn hình hoặc nhấn nút USER (PA0) chuyển đổi giữa Screen1 và Screen2; đồ thị lịch sử được giữ nguyên nhờ bộ đệm 160 mẫu trong Model.

Nếu cảm biến hoặc RTC bị rút ra giữa chừng, hệ thống tự dò lại theo chu kỳ (3 giây cho MAX30102, 15 giây cho RTC sau 5 giây khởi động) thay vì treo hoặc yêu cầu reset thủ công.

---

## ĐẶC TẢ HÀM

### 1. Phát hiện ngón tay ([Core/Src/finger_detector.c](../Core/Src/finger_detector.c))
```c
bool FingerDetector_Update(FingerDetector *detector,
                           uint32_t ir,
                           uint32_t red)
{
    ...
    pulse_candidate = dc_valid &&
                      (detector->span_ir >= ir_span_required);

    if (detector->baseline_valid) {
        step_required = max_u32(DETECTOR_BASELINE_STEP_MIN,
                                detector->baseline_ir /
                                DETECTOR_BASELINE_STEP_DIVISOR);
        detector->threshold_ir = detector->baseline_ir + step_required;
        step_candidate = dc_valid &&
                         (abs_diff_u32(detector->mean_ir,
                                       detector->baseline_ir) >= step_required);
    }

    strong_candidate = dc_valid &&
                       (detector->mean_ir >= DETECTOR_STRONG_IR_LEVEL) &&
                       (detector->span_ir >= 2U);

    candidate = pulse_candidate || step_candidate || strong_candidate;
    ...
}
```
**Mô tả:** Giữ cửa sổ trượt 16 mẫu Red/IR gần nhất, học baseline "chưa có tay" bằng bộ lọc EMA khi các mẫu vẫn trông giống không khí, rồi coi là có ngón tay khi biên độ IR đủ lớn, hoặc DC nhảy đủ xa khỏi baseline, hoặc mức IR đủ mạnh. Cần 2 mẫu liên tiếp thoả điều kiện mới xác nhận có tay và 20 mẫu liên tiếp mất tín hiệu mới xác nhận mất tay, tránh nhận nhầm do nhiễu tức thời.

### 2. Tính nhịp tim bằng tự tương quan ([Core/Src/spo2_algorithm.c](../Core/Src/spo2_algorithm.c))
```c
static bool estimate_heart_rate(const float *ir_ac,
                                uint16_t length,
                                float sample_rate_hz,
                                float *heart_rate,
                                float *correlation)
{
    uint16_t min_lag = (uint16_t)(sample_rate_hz * 60.0f / 220.0f);
    uint16_t max_lag = (uint16_t)(sample_rate_hz * 60.0f / 35.0f);
    ...
    for (lag = min_lag; lag <= max_lag; ++lag) {
        corr[lag] = normalized_autocorrelation(ir_ac, length, lag);
        if (corr[lag] > best_corr) {
            best_corr = corr[lag];
            best_lag = lag;
        }
    }
    /* Prefer the first strong local peak so a harmonic is not selected. */
    for (lag = (uint16_t)(min_lag + 1U); lag < max_lag; ++lag) {
        if ((corr[lag] >= corr[lag - 1U]) && (corr[lag] >= corr[lag + 1U]) &&
            (corr[lag] >= best_corr * 0.85f) && (corr[lag] > 0.20f)) {
            selected_lag = lag;
            break;
        }
    }
    ...
    *heart_rate = 60.0f * sample_rate_hz / refined_lag;
    return (*heart_rate >= 35.0f) && (*heart_rate <= 220.0f);
}
```
**Mô tả:** Tính tự tương quan chuẩn hoá của thành phần AC kênh IR tại các độ trễ tương ứng dải nhịp tim 35–220 bpm, ưu tiên đỉnh tương quan sớm nhất để tránh chọn nhầm sóng hài, rồi tinh chỉnh vị trí đỉnh bằng nội suy parabol. Nếu không hội tụ, `spo2_algorithm.c` dùng thêm phương án dự phòng đếm đỉnh (`estimate_heart_rate_peaks`).

### 3. Tự động chỉnh dòng LED ([Core/Src/spo2_app.c](../Core/Src/spo2_app.c))
```c
static bool auto_adjust_led_current(uint32_t now)
{
    uint32_t mean_ir = FingerDetector_GetMeanIR(&finger_detector);
    uint8_t new_current = sensor_led_current;

    if ((now - last_gain_adjust_tick) < SENSOR_GAIN_ADJUST_INTERVAL_MS ||
        finger_detector.sample_count < 8U) {
        return false;
    }

    if ((mean_ir >= SENSOR_SIGNAL_SATURATED) &&
        (sensor_led_current > SENSOR_LED_CURRENT_MIN)) {
        new_current = (sensor_led_current > 0x10U + SENSOR_LED_CURRENT_MIN)
                          ? (uint8_t)(sensor_led_current - 0x10U)
                          : SENSOR_LED_CURRENT_MIN;
    } else if (!detector_contact && !algorithm_contact &&
               (mean_ir < SENSOR_SIGNAL_TOO_LOW) &&
               (sensor_led_current < SENSOR_LED_CURRENT_MAX)) {
        uint16_t raised = (uint16_t)sensor_led_current + 0x20U;
        new_current = (raised > SENSOR_LED_CURRENT_MAX)
                          ? SENSOR_LED_CURRENT_MAX : (uint8_t)raised;
    }
    ...
}
```
**Mô tả:** Mỗi ~2 giây, giảm dòng LED khi IR trung bình vượt ngưỡng bão hoà (`>= 245000`), hoặc tăng dòng khi chưa phát hiện tiếp xúc và IR trung bình quá thấp (`< 120`), trong khoảng `0x20`–`0x7F`. Mỗi lần đổi dòng LED, vòng đệm đo và bộ dò ngón tay được reset để tránh trộn hai mức tín hiệu khác nhau trong cùng một cửa sổ tính toán.

### 4. Ép cấu hình lại khi FIFO ngừng sinh dữ liệu ([Core/Src/max30102.c](../Core/Src/max30102.c))
```c
MAX30102_Status MAX30102_ForceMeasurement(MAX30102_Handle *dev,
                                           uint8_t led_current)
{
    MAX30102_Status status;
    ...
    status = write_reg(dev, REG_MODE_CONFIG, 0x00U);
    if (status != MAX30102_OK) return status;
    HAL_Delay(2U);

    if (write_reg(dev, REG_INTR_ENABLE_1, 0x00U) != MAX30102_OK ||
        write_reg(dev, REG_INTR_ENABLE_2, 0x00U) != MAX30102_OK) {
        return MAX30102_ERROR_I2C;
    }
    if (write_reg(dev, REG_FIFO_CONFIG, 0x1FU) != MAX30102_OK) return MAX30102_ERROR_I2C;
    if (write_reg(dev, REG_MODE_CONFIG, MODE_SPO2) != MAX30102_OK) return MAX30102_ERROR_I2C;
    if (write_reg(dev, REG_SPO2_CONFIG, 0x27U) != MAX30102_OK) return MAX30102_ERROR_I2C;
    if (MAX30102_SetLedCurrent(dev, led_current, led_current) != MAX30102_OK) return MAX30102_ERROR_I2C;
    if (MAX30102_ClearFIFO(dev) != MAX30102_OK) return MAX30102_ERROR_I2C;

    HAL_Delay(160U);
    return MAX30102_OK;
}
```
**Mô tả:** Được gọi khi FIFO không sinh mẫu mới trong hơn 1,5 giây dù cảm biến vẫn trả lời trên bus. Nạp lại một cấu hình đo SpO₂ tối thiểu đã biết là hoạt động (thoát shutdown, tắt ngắt, cấu hình FIFO/mode/SpO2, đặt lại dòng LED, xoá FIFO) để phục hồi các module MAX30102 giá rẻ bị "treo" phần cấu hình mà không cần rút cắm lại.

### 5. Tổng hợp kết quả đo và cập nhật trạng thái ([Core/Src/spo2_app.c](../Core/Src/spo2_app.c))
```c
static void calculate_measurement(void)
{
    SpO2AlgorithmResult result;
    ...
    linearize_buffers();
    SpO2Algorithm_Compute(ir_linear, red_linear, SPO2_APP_BUFFER_LENGTH,
                          SPO2_APP_FIFO_SAMPLE_RATE_HZ, 0U, &result);

    current.signal_quality = result.signal_quality;
    current.heart_rate_valid = result.heart_rate_valid;
    current.spo2_valid = result.spo2_valid;
    current.measurement_valid = current.heart_rate_valid && current.spo2_valid;
    ...
    current.low_spo2 = current.spo2_percent < SPO2_APP_LOW_SPO2_THRESHOLD;
    current.abnormal_heart_rate =
        (current.heart_rate_bpm < low_hr) || (current.heart_rate_bpm > high_hr);

    if (current.low_spo2) {
        current.status = SPO2_APP_LOW_SPO2;
    } else if (current.heart_rate_bpm < low_hr) {
        current.status = SPO2_APP_LOW_HEART_RATE;
    } else if (current.heart_rate_bpm > high_hr) {
        current.status = SPO2_APP_HIGH_HEART_RATE;
    } else {
        current.status = SPO2_APP_NORMAL;
    }
}
```
**Mô tả:** Chạy mỗi 25 mẫu mới trên cửa sổ trượt 100 mẫu (~4 giây). Gọi thuật toán PPG, ghi nhận BPM/SpO₂ là hai giá trị hợp lệ độc lập, so sánh SpO₂ với ngưỡng cảnh báo (`< 85%`) và nhịp tim với dải thích nghi theo trung bình trượt của chính người đo, rồi suy ra một trong các trạng thái `NORMAL`/`LOW_SPO2`/`LOW_HEART_RATE`/`HIGH_HEART_RATE`/`MEASURING`/`INVALID_SIGNAL`.

### 6. Cầu nối dữ liệu Core → TouchGFX ([Model.cpp](../TouchGFX/gui/src/model/Model.cpp))
```cpp
void Model::tick()
{
    if (modelListener == 0) return;
    ...
    SpO2AppSnapshot snapshot;
    SpO2App_GetSnapshot(&snapshot);

    if (snapshot.sequence == lastSequence) return;
    lastSequence = snapshot.sequence;

    SpO2UiData data = {};
    data.sequence = snapshot.sequence;
    data.status = mapStatus(snapshot.status);
    data.heartRateBpm = snapshot.heart_rate_bpm;
    data.spo2Percent = snapshot.spo2_percent;
    ...
    appendGraphSample(data);
    modelListener->onSpO2DataChanged(data);
}
```
**Mô tả:** Chạy mỗi tick TouchGFX, so sánh số thứ tự `sequence` của snapshot để biết có dữ liệu mới từ `defaultTask` hay không (tránh vẽ lại khi không có gì thay đổi), chuyển đổi sang kiểu `SpO2UiData` cho GUI, đồng thời gọi `appendGraphSample()` để lưu vào bộ đệm lịch sử 160 mẫu phục vụ Screen2.

### 7. Cập nhật hiển thị Screen1 ([Screen1View.cpp](../TouchGFX/gui/src/screen1_screen/Screen1View.cpp))
```cpp
void Screen1View::updateData(const SpO2UiData& data)
{
    if (data.heartRateValid) {
        Unicode::snprintf(textBpmBuffer, TEXTBPM_SIZE, "%d", data.heartRateBpm);
    } else {
        Unicode::fromUTF8((const uint8_t*)"--", textBpmBuffer, TEXTBPM_SIZE);
    }

    if (data.spo2Valid) {
        Unicode::snprintf(textSpO2Buffer, TEXTSPO2_SIZE, "%d", data.spo2Percent);
    } else {
        Unicode::fromUTF8((const uint8_t*)"--", textSpO2Buffer, TEXTSPO2_SIZE);
    }
    ...
    if ((data.status == SPO2_UI_SENSOR_ERROR) || (data.status == SPO2_UI_LOW_SPO2) ||
        (data.status == SPO2_UI_LOW_HEART_RATE) || (data.status == SPO2_UI_HIGH_HEART_RATE)) {
        textStatus.setColor(touchgfx::Color::getColorFromRGB(255, 96, 96));
    } else if (data.status == SPO2_UI_NORMAL) {
        textStatus.setColor(touchgfx::Color::getColorFromRGB(90, 230, 140));
    } else {
        textStatus.setColor(touchgfx::Color::getColorFromRGB(255, 210, 80));
    }
}
```
**Mô tả:** BPM và SpO₂ được vẽ độc lập — mỗi giá trị hiển thị `--` riêng nếu chưa hợp lệ thay vì số 0 gây hiểu nhầm. Màu chữ trạng thái đổi theo mức độ: đỏ cho lỗi/cảnh báo, xanh lá cho `NORMAL`, vàng cho các trạng thái trung gian (`PLACE FINGER`, `MEASURING...`).

### 8. Vòng lặp chính của task FreeRTOS ([Core/Src/main.c](../Core/Src/main.c))
```c
void StartDefaultTask(void *argument)
{
  osDelay(1500U); /* nhường TouchGFX vẽ khung hình đầu tiên trước */

  I2C3_BusLock();
  I2C3_BusRecover();
  SpO2App_Init(&hi2c3, SPO2_RTC_KIND);
  I2C3_BusUnlock();

  for (;;)
  {
    I2C3_BusLock();
    SpO2App_Process();
    I2C3_BusUnlock();

    osDelay(20U);
  }
}
```
**Mô tả:** Luồng thực thi chính của tầng đo, chạy song song với GUI task của TouchGFX. Chờ 1,5 giây để GUI vẽ khung hình đầu tiên trước khi chạm vào I²C, sau đó khoá bus, phục hồi bus (giải quyết SDA/SCL bị kẹt), khởi tạo `SpO2App`, rồi lặp vô hạn gọi `SpO2App_Process()` mỗi 20 ms trong một khoá bus ngắn — không bao giờ chặn vòng vẽ GUI dù cảm biến lỗi hay bị rút.

---

## KẾT QUẢ

### Minh chứng sản phẩm

| Tình huống | Kết quả trên Screen1 |
| --- | --- |
| Chưa đặt ngón tay | `--`/`--`, trạng thái `PLACE FINGER` |
| Vừa đặt tay, chưa đủ dữ liệu | `MEASURING...` |
| BPM đã hợp lệ, SpO₂ vẫn đang chờ kênh Red | BPM hiển thị số, SpO₂ vẫn `--` |
| Đo ổn định, trong ngưỡng | Giá trị BPM/SpO₂, trạng thái `NORMAL` (chữ xanh lá) |
| SpO₂ < 85% hoặc nhịp tim bất thường | `LOW SPO2` / `LOW HEART RATE` / `HIGH HEART RATE` (chữ đỏ) |
| Lỗi/rút cảm biến | Mã lỗi chi tiết (`MAX30102 MISSING`, `SDA STUCK LOW`...), tự dò lại định kỳ |
| FIFO cảm biến ngừng sinh mẫu dù vẫn trả lời trên bus | Tự động force-restart cấu hình đo |
| Mất RTC | Vẫn đo được BPM/SpO₂; thời gian dùng đồng hồ phần mềm dự phòng |
| Chuyển qua lại Screen1 ↔ Screen2 | Đồ thị lịch sử BPM/SpO₂ không bị mất |

Kiểm thử đã thực hiện: `tests/test_spo2_algorithm.c` (tín hiệu PPG mô phỏng 75 bpm/97% SpO₂), `tests/test_spo2_algorithm_low_signal.c` (tín hiệu biên độ thấp), `tests/test_finger_detector.c` (4 kịch bản phát hiện ngón tay). Báo cáo kiểm thử tĩnh/render headless cho bản v8: [`docs/VALIDATION_REPORT.md`](../docs/VALIDATION_REPORT.md).


### Hạn chế

- Công thức SpO₂ (`-45.060R² + 30.354R + 94.845`) và ngưỡng cảnh báo `< 85%` là các giá trị thực nghiệm, **chưa hiệu chuẩn bằng thiết bị y tế chuẩn** — không dùng để chẩn đoán.
- Thuật toán tự tương quan nhạy với cử động mạnh của ngón tay; cần giữ tay tương đối yên trong vài giây đầu.
- Cơ chế force-restart FIFO khắc phục lỗi cấu hình/mềm nhưng không khắc phục được lỗi phần cứng thật (nguồn, AFE) của module MAX30102 giá rẻ.
- Nếu module Tiny RTC là DS1307 gốc thiết kế cho 5 V, cần xử lý đúng mức điện áp bus để không ảnh hưởng chân I²C 3,3 V của STM32.
