# HƯỚNG DẪN TẠO GIAO DIỆN TOUCHGFX 2 MÀN HÌNH
**BTL SpO₂ — STM32F429ZIT6 + MAX30102 + Tiny RTC**

Tài liệu này bổ sung cho `Huong_dan_lam_Project_SpO2_STM32F429ZIT6_TouchGFX_3Module.md`.
Mục tiêu: dựng **2 màn hình** trong TouchGFX Designer, chuyển bằng nút **B1** (User
Button PA0), luôn stream dữ liệu qua **USART3**, và **nháy LED PG13/PG14** khi
BPM/SpO₂ vượt ngưỡng.

- **Screen1 — Digital**: hiển thị BPM và SpO₂ dạng số cỡ lớn (giữ nguyên UI đã có).
- **Screen2 — Graph**: 2 đồ thị line real-time cho BPM và SpO₂.

---

## 1. Tổng quan 2 màn hình

Kit STM32F429I‑DISC1 có LCD 240×320 dọc. Bố cục gợi ý cho từng màn:

| Màn hình | Widget chính | Wildcard buffer | Ghi chú |
|---|---|---|---|
| Screen1 | `textSpo2`, `textBpm` | `textSpo2Buffer` (20), `textBpmBuffer` (20) | Font 60px |
| Screen1 | `textTime`, `textDate` | `textTimeBuffer` (32), `textDateBuffer` (32) | Font 20px |
| Screen1 | `textStatus` | `textStatusBuffer` (40) | Font 20px |
| Screen2 | `graphBpm` (Dynamic Graph) | — | 100 điểm, Y = 40..180, màu đỏ |
| Screen2 | `graphSpo2` (Dynamic Graph) | — | 100 điểm, Y = 70..100, màu xanh dương |
| Screen2 | `textBpmMini`, `textSpo2Mini` | `textBpmMiniBuffer` (10), `textSpo2MiniBuffer` (10) | Font 20px, hiển thị giá trị hiện tại |
| Screen2 | `textStatus2` | `textStatus2Buffer` (40) | Font 16px |

> Giữ nguyên tên buffer Screen1 khớp với `code/TouchGFX/README_TouchGFX_UI.md` và
> `code/TouchGFX/gui/src/screen1_screen/Screen1View.cpp` — nếu đổi tên phải sửa
> lại file cpp tương ứng.

---

## 2. Thao tác trong TouchGFX Designer

### 2.1 Screen1 — Digital
Tạo trước theo hướng dẫn ở tài liệu gốc (mục 5). Bố cục tham khảo:

```
+------------------------------+  y=0
|      HEALTH MONITOR          |
+------------------------------+  y=32
|                              |
|   SpO2          BPM          |
|   [ 98 % ]     [ 78 BPM ]    |  font 60px, y=60..160
|                              |
+------------------------------+
|   14:35:20   24/06/2026      |  font 20px, y=190
+------------------------------+
|          Normal              |  textStatus, font 20px, y=280
+------------------------------+
```

### 2.2 Screen2 — Graph

**Bước A. Thêm màn hình mới**
1. Trong Designer, chọn `+ Add Screen` → đặt tên **Screen2** → Blank.
2. **Không** đặt Screen2 làm start screen (giữ Screen1 là start).

**Bước B. Thêm khung nền**
1. Kéo Box vào Screen2, kích thước 240×280, đặt ở `(0, 32)`, màu `#101820`.

**Bước C. Kéo Dynamic Graph cho BPM**
1. Từ palette *Miscellaneous* → **Dynamic Graph** (nếu không thấy, cập nhật Designer ≥ 4.18).
2. Đặt tên **graphBpm**, size `240×110`, vị trí `(0, 40)`.
3. Cấu hình trong panel Properties:
   - `Type`: **Wrap and Clear**
   - `Points`: **100**
   - `Y min = 40`, `Y max = 180`
   - `X spacing`: `2.4`
   - `Graph Line Color`: đỏ `#E74C3C`, độ dày 2
   - Bật `Y-Axis` (optional)

**Bước D. Kéo Dynamic Graph cho SpO₂**
1. **Dynamic Graph** thứ 2, tên **graphSpo2**, size `240×110`, vị trí `(0, 160)`.
2. Properties:
   - `Type`: Wrap and Clear, `Points`: 100
   - `Y min = 70`, `Y max = 100`
   - `Graph Line Color`: xanh `#3498DB`, dày 2

**Bước E. Thêm TextArea nhỏ**
1. TextArea **textBpmMini**, wildcard `textBpmMiniBuffer` (10 ký tự), font 20, đặt góc trên phải `graphBpm`.
2. TextArea **textSpo2Mini**, wildcard `textSpo2MiniBuffer` (10 ký tự), font 20, đặt góc trên phải `graphSpo2`.
3. TextArea **textStatus2**, wildcard `textStatus2Buffer` (40 ký tự), font 16, ở đáy màn hình.

**Bước F. KHÔNG cần Interaction chuyển màn**
Nút B1 là GPIO cứng, không phải touch → chuyển màn được thực hiện bằng code trong
`Model.cpp` gọi `application().gotoScreen1ScreenNoTransition()` /
`gotoScreen2ScreenNoTransition()`. Bỏ qua tab Interactions.

**Bước G. Generate code**
Bấm `Generate Code` trong Designer. Sẽ có 2 cặp file mới:
```
TouchGFX/gui/include/gui/screen2_screen/Screen2View.hpp
TouchGFX/gui/include/gui/screen2_screen/Screen2Presenter.hpp
TouchGFX/gui/src/screen2_screen/Screen2View.cpp
TouchGFX/gui/src/screen2_screen/Screen2Presenter.cpp
```

---

## 3. Cấu hình CubeMX bổ sung

Ngoài phần đã có trong tài liệu gốc (LTDC, SDRAM, DMA2D, I2C1), bật thêm:

| Peripheral | Cấu hình | Mục đích |
|---|---|---|
| **GPIO PA0** | Mode = `GPIO_EXTI0`, Pull = **Pull-down**, Trigger = **Rising** | Nút B1 |
| **NVIC** | Enable `EXTI Line0 interrupt`, priority `5` (thấp hơn TouchGFX) | ISR cho PA0 |
| **GPIO PG13** | Output Push-pull, No pull, Low speed, Init Low | LED xanh (LD3) |
| **GPIO PG14** | Output Push-pull, No pull, Low speed, Init Low | LED đỏ (LD4) |
| **USART3** | Async, 115200 8N1, TX = **PD8**, RX = **PD9** | Stream dữ liệu về PC |

Generate code lại. Kiểm tra `main.c` phải có: `MX_USART3_UART_Init()`,
`MX_GPIO_Init()` với PG13/PG14 = output và PA0 = EXTI0.

---

## 4. Dán code

### 4.1 `Core/Src/main.c`

**a. USER CODE BEGIN Includes** (trên cùng):
```c
#include "health_monitor.h"
#include <stdio.h>
#include <string.h>
```

**b. USER CODE BEGIN PD** (defines):
```c
#define BPM_LOW      50
#define BPM_HIGH     120
#define SPO2_LOW     92
#define ALARM_BLINK_MS 250
```

**c. USER CODE BEGIN 2** (sau khi tất cả `MX_*_Init()` đã chạy):
```c
HealthMonitor_Init(&hi2c1);
```

**d. USER CODE BEGIN WHILE** (thay thế loop trống):
```c
uint32_t lastProc = 0, lastTx = 0, lastBlink = 0;
uint8_t alarm_on = 0, blink_state = 0;

while (1) {
    uint32_t now = HAL_GetTick();

    /* Xử lý sensor mỗi 10 ms */
    if (now - lastProc >= 10) {
        lastProc = now;
        HealthMonitor_Process10ms();
    }

    HealthData_t d = HealthMonitor_GetData();

    /* Kiểm ngưỡng */
    alarm_on = 0;
    if (d.valid) {
        if (d.bpm < BPM_LOW || d.bpm > BPM_HIGH) alarm_on = 1;
        if (d.spo2 < SPO2_LOW)                   alarm_on = 1;
    }

    /* Nháy LED PG13/PG14 */
    if (alarm_on) {
        if (now - lastBlink >= ALARM_BLINK_MS) {
            lastBlink = now;
            blink_state ^= 1;
            HAL_GPIO_WritePin(GPIOG, GPIO_PIN_13, blink_state ? GPIO_PIN_SET : GPIO_PIN_RESET);
            HAL_GPIO_WritePin(GPIOG, GPIO_PIN_14, blink_state ? GPIO_PIN_SET : GPIO_PIN_RESET);
        }
    } else {
        HAL_GPIO_WritePin(GPIOG, GPIO_PIN_13, GPIO_PIN_RESET);
        HAL_GPIO_WritePin(GPIOG, GPIO_PIN_14, GPIO_PIN_RESET);
        blink_state = 0;
    }

    /* Stream USART3 mỗi 200 ms */
    if (now - lastTx >= 200) {
        lastTx = now;
        char line[64];
        int n = snprintf(line, sizeof line,
                         "%ld,%ld,%u,%s\r\n",
                         (long)d.bpm, (long)d.spo2,
                         (unsigned)d.status,
                         alarm_on ? "ALARM" : "OK");
        HAL_UART_Transmit(&huart3, (uint8_t*)line, n, 50);
    }
}
```

### 4.2 `Core/Src/stm32f4xx_it.c`

**a. USER CODE BEGIN 0** (trên cùng):
```c
volatile uint8_t g_btn_toggle_flag = 0;
```

**b. Cuối file, USER CODE BEGIN 1**:
```c
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin) {
    if (GPIO_Pin == GPIO_PIN_0) {
        static uint32_t last = 0;
        uint32_t now = HAL_GetTick();
        if (now - last > 200) {            /* debounce 200 ms */
            g_btn_toggle_flag = 1;
            last = now;
        }
    }
}
```

### 4.3 `TouchGFX/gui/src/model/Model.cpp`

Ghi đè toàn bộ nội dung bằng:

```cpp
#include <gui/model/Model.hpp>
#include <gui/model/ModelListener.hpp>
#include <mvp/Application.hpp>

extern "C" {
#include "health_monitor.h"
extern volatile uint8_t g_btn_toggle_flag;
}

static uint8_t s_currentScreen = 1;  /* 1 = Screen1, 2 = Screen2 */

Model::Model() : modelListener(0) {}

void Model::tick()
{
    /* --- Chuyển màn khi B1 được nhấn --- */
    if (g_btn_toggle_flag) {
        g_btn_toggle_flag = 0;
        if (modelListener) {
            modelListener->screenToggleRequested();
        }
    }

    /* --- Cập nhật GUI ~5 Hz --- */
    static int tickDiv = 0;
    if (++tickDiv >= 12) {
        tickDiv = 0;
        HealthData_t data = HealthMonitor_GetData();
        if (modelListener) {
            modelListener->healthDataChanged(data);
        }
    }
}

/* Helper cho Presenter đọc/ghi màn hình hiện tại */
uint8_t Model_GetCurrentScreen(void) { return s_currentScreen; }
void    Model_SetCurrentScreen(uint8_t s) { s_currentScreen = s; }
```

### 4.4 `TouchGFX/gui/include/gui/model/ModelListener.hpp`

Thêm hàm ảo:

```cpp
#ifndef MODELLISTENER_HPP
#define MODELLISTENER_HPP

#include <gui/model/Model.hpp>
extern "C" {
#include "health_monitor.h"
}

class ModelListener
{
public:
    ModelListener() : model(0) {}
    virtual ~ModelListener() {}
    void bind(Model* m) { model = m; }
    virtual void healthDataChanged(const HealthData_t& data) {}
    virtual void screenToggleRequested() {}
protected:
    Model* model;
};

#endif
```

### 4.5 `TouchGFX/gui/src/screen1_screen/Screen1Presenter.cpp`

Thêm override:

```cpp
#include <gui/screen1_screen/Screen1View.hpp>
#include <gui/screen1_screen/Screen1Presenter.hpp>
#include <mvp/Application.hpp>

extern "C" {
uint8_t Model_GetCurrentScreen(void);
void    Model_SetCurrentScreen(uint8_t s);
}

Screen1Presenter::Screen1Presenter(Screen1View& v) : view(v) {}
void Screen1Presenter::activate()   { Model_SetCurrentScreen(1); }
void Screen1Presenter::deactivate() {}

void Screen1Presenter::healthDataChanged(const HealthData_t& data)
{
    view.updateHealthData(data);
}

void Screen1Presenter::screenToggleRequested()
{
    static_cast<FrontendApplication*>(Application::getInstance())
        ->gotoScreen2ScreenNoTransition();
}
```

Nhớ khai báo `virtual void screenToggleRequested();` trong
`Screen1Presenter.hpp` (đã có `ModelListener` khai báo virtual, đây chỉ là
override).

### 4.6 `TouchGFX/gui/src/screen2_screen/Screen2View.cpp`

Sau khi Designer sinh khung, mở file và bổ sung:

```cpp
#include <gui/screen2_screen/Screen2View.hpp>
#include <touchgfx/Unicode.hpp>
extern "C" { #include "health_monitor.h" }

Screen2View::Screen2View() {}
void Screen2View::setupScreen()   { Screen2ViewBase::setupScreen(); }
void Screen2View::tearDownScreen(){ Screen2ViewBase::tearDownScreen(); }

void Screen2View::updateHealthData(const HealthData_t& data)
{
    if (data.valid) {
        graphBpm.addValue(data.bpm);
        graphSpo2.addValue(data.spo2);
        Unicode::snprintf(textBpmMiniBuffer,  TEXTBPMMINI_SIZE,  "%ld", (long)data.bpm);
        Unicode::snprintf(textSpo2MiniBuffer, TEXTSPO2MINI_SIZE, "%ld%%", (long)data.spo2);
    } else {
        Unicode::snprintf(textBpmMiniBuffer,  TEXTBPMMINI_SIZE,  "--");
        Unicode::snprintf(textSpo2MiniBuffer, TEXTSPO2MINI_SIZE, "--%%");
    }
    Unicode::snprintf(textStatus2Buffer, TEXTSTATUS2_SIZE, "%s",
                      HealthMonitor_StatusText(data.status));

    textBpmMini.invalidate();
    textSpo2Mini.invalidate();
    textStatus2.invalidate();
    graphBpm.invalidate();
    graphSpo2.invalidate();
}
```

Và ở `Screen2View.hpp`:

```cpp
#include <gui_generated/screen2_screen/Screen2ViewBase.hpp>
#include <gui/screen2_screen/Screen2Presenter.hpp>
extern "C" { #include "health_monitor.h" }

class Screen2View : public Screen2ViewBase {
public:
    Screen2View();
    virtual ~Screen2View() {}
    virtual void setupScreen();
    virtual void tearDownScreen();
    void updateHealthData(const HealthData_t& data);
};
```

### 4.7 `TouchGFX/gui/src/screen2_screen/Screen2Presenter.cpp`

```cpp
#include <gui/screen2_screen/Screen2View.hpp>
#include <gui/screen2_screen/Screen2Presenter.hpp>
#include <mvp/Application.hpp>

extern "C" {
void Model_SetCurrentScreen(uint8_t s);
}

Screen2Presenter::Screen2Presenter(Screen2View& v) : view(v) {}
void Screen2Presenter::activate()   { Model_SetCurrentScreen(2); }
void Screen2Presenter::deactivate() {}

void Screen2Presenter::healthDataChanged(const HealthData_t& data)
{
    view.updateHealthData(data);
}

void Screen2Presenter::screenToggleRequested()
{
    static_cast<FrontendApplication*>(Application::getInstance())
        ->gotoScreen1ScreenNoTransition();
}
```

Khai báo trong `Screen2Presenter.hpp` (giống file Screen1Presenter.hpp cũ, thêm
`virtual void screenToggleRequested();`).

---

## 5. Test end-to-end

| Bước | Cách test | Đạt khi |
|---|---|---|
| 1. Screen1 chạy | Nạp firmware, đặt tay lên MAX30102 | Số BPM/SpO₂ hiện đúng |
| 2. Bấm B1 | Nhấn nút xanh (PA0) trên DISC1 | Chuyển sang Screen2 |
| 3. Đồ thị vẽ | Chờ 5–10 giây | 2 đường line dịch từ phải qua trái |
| 4. Bấm B1 lần 2 | Nhấn lại | Về Screen1 |
| 5. USART | Mở PuTTY/TeraTerm 115200 8N1 trên COMx của ST-LINK VCP | Xuất dòng `bpm,spo2,status,OK\|ALARM\r\n` mỗi 200 ms |
| 6. Cảnh báo LED | Bỏ tay khỏi cảm biến hoặc đặt lỏng để SpO₂ < 92 | PG13 + PG14 nháy đồng thời ~2 Hz |
| 7. Hết cảnh báo | Đặt tay chuẩn lại | LED tắt hẳn |

---

## 6. Ngưỡng y tế cố định

| Chỉ số | Ngưỡng | Nguồn tham khảo |
|---|---|---|
| Nhịp tim thấp (bradycardia) | `< 50 BPM` | AHA (người lớn nghỉ ngơi) |
| Nhịp tim cao (tachycardia) | `> 120 BPM` | AHA (nghỉ ngơi, không vận động) |
| SpO₂ thấp (hypoxemia) | `< 92 %` | WHO, hướng dẫn oxygen therapy |

> Cảnh báo: các ngưỡng và công thức SpO₂ trong project chỉ dùng cho mục đích
> **học tập**, không thay thế thiết bị y tế đã kiểm chuẩn.
