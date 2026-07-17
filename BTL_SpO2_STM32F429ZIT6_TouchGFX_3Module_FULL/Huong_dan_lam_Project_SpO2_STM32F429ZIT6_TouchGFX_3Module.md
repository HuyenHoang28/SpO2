**HƯỚNG DẪN LÀM PROJECT SpO₂
STM32F429ZIT6 + MAX30102 + TINY RTC + TOUCHGFX**

Tài liệu này hướng dẫn triển khai project từ đầu với đúng 3 module phần cứng: kit STM32F429ZIT6 có LCD/TFT, MAX30102 và Tiny RTC. Không dùng OLED, không dùng LED rời, không dùng ESP32/Blynk.

# 1. Kiến trúc cuối cùng

| **Thành phần** | **Vai trò** | **Ghi chú** |
| --- | --- | --- |
| STM32F429ZIT6 kit | Đọc cảm biến, xử lý thuật toán, chạy TouchGFX | LCD/TFT nằm trên kit |
| MAX30102 | Đọc tín hiệu Red/IR để tính BPM và SpO₂ | Địa chỉ I2C 0x57 |
| Tiny RTC DS3231/DS1307 | Cấp thời gian thực cho màn hình | Địa chỉ I2C 0x68 |
| TouchGFX | Hiển thị BPM, SpO₂, ngày giờ, trạng thái | Là phần mềm GUI, không phải module ngoài |

Luồng dữ liệu: MAX30102/Tiny RTC -> I2C1 -> STM32F429ZIT6 -> HealthMonitor -> TouchGFX Model/Presenter/View -> LCD.

# 2. Đấu nối phần cứng

| **STM32F429ZIT6** | **MAX30102** | **Tiny RTC** | **Ghi chú** |
| --- | --- | --- | --- |
| 3.3V | VCC/VIN | VCC | Ưu tiên dùng 3.3V |
| GND | GND | GND | Bắt buộc nối chung mass |
| PB8/I2C1_SCL | SCL | SCL | Clock I2C |
| PB9/I2C1_SDA | SDA | SDA | Data I2C |

Không cấp 5V vào SDA/SCL của STM32.

Nếu Tiny RTC là DS1307 5V, kiểm tra điện trở kéo lên; nên dùng DS3231 3.3V.

Khi mới test, chỉ nối I2C và nguồn, chưa cần thêm bất cứ module nào khác.

# 3. Tạo project TouchGFX rỗng

Mở STM32CubeIDE và tạo project mới.

Chọn đúng board STM32F429ZIT6/STM32F429I-DISC1 nếu có trong Board Selector.

Bật TouchGFX middleware. Với board có LCD, giữ cấu hình template cho LTDC, DMA2D, FMC/SDRAM.

Bật CRC nếu CubeMX yêu cầu.

Generate code, build và nạp thử. Màn hình phải chạy trước khi thêm cảm biến.

# 4. Cấu hình I2C1

Bật I2C1 trong CubeMX.

Chọn PB8 = I2C1_SCL, PB9 = I2C1_SDA.

Speed mode: Standard 100 kHz lúc mới test.

Generate code lại.

Kiểm tra trong main.c có MX_I2C1_Init() và biến hi2c1.

// Test nhanh thiết bị trên bus I2C
uint8_t ok_max = HAL_I2C_IsDeviceReady(&hi2c1, 0x57 << 1, 3, 100) == HAL_OK;
uint8_t ok_rtc = HAL_I2C_IsDeviceReady(&hi2c1, 0x68 << 1, 3, 100) == HAL_OK;

# 5. Thiết kế giao diện TouchGFX

| **TextArea** | **Wildcard buffer** | **Nội dung** |
| --- | --- | --- |
| textSpo2 | textSpo2Buffer | -- % hoặc 98 % |
| textBpm | textBpmBuffer | -- BPM hoặc 78 BPM |
| textTime | textTimeBuffer | 14:35:20 |
| textDate | textDateBuffer | 24/06/2026 |
| textStatus | textStatusBuffer | Place finger / Measuring / Normal / Warning |

Tạo Screen1 trong TouchGFX Designer.

Thêm các TextArea trên và bật wildcard.

Generate code từ TouchGFX Designer.

Nếu tên widget khác, sửa lại Screen1View.cpp theo đúng tên của bạn.

# 6. Copy code vào project

| **Thư mục trong gói code** | **Copy tới đâu** |
| --- | --- |
| Core/Inc/*.h | Core/Inc của project CubeIDE |
| Core/Src/*.c | Core/Src của project CubeIDE |
| TouchGFX/gui/include/gui/model | gui/include/gui/model |
| TouchGFX/gui/src/model | gui/src/model |
| TouchGFX/gui/include/gui/screen1_screen | gui/include/gui/screen1_screen |
| TouchGFX/gui/src/screen1_screen | gui/src/screen1_screen |

main_integration_example.c chỉ là file hướng dẫn chèn code, không đưa vào build nếu đã có main.c do CubeMX sinh ra.

// Chèn trong main.c sau MX_I2C1_Init()
#include "health_monitor.h"
extern I2C_HandleTypeDef hi2c1;

HealthMonitor_Init(&hi2c1);

// Gọi mỗi 10ms nếu project không dùng FreeRTOS
static uint32_t lastTick = 0;
if (HAL_GetTick() - lastTick >= 10) {
    lastTick = HAL_GetTick();
    HealthMonitor_Process10ms();
}

# 7. Test theo thứ tự

| **Bước** | **Cách test** | **Đạt khi** |
| --- | --- | --- |
| 1. TouchGFX | Nạp project rỗng | LCD hiển thị giao diện |
| 2. I2C | HAL_I2C_IsDeviceReady | Thấy 0x57 và 0x68 |
| 3. MAX30102 | Đọc PART_ID | Giá trị 0x15 |
| 4. RTC | Đọc giây liên tục | Giây tăng dần |
| 5. Raw Red/IR | Đọc FIFO | IR tăng khi đặt tay |
| 6. Thuật toán | Đặt tay 10-15s | Có BPM/SpO₂ |
| 7. GUI | Quan sát LCD | TextArea cập nhật đúng |

# 8. Lỗi thường gặp và cách sửa

| **Lỗi** | **Nguyên nhân hay gặp** | **Cách sửa** |
| --- | --- | --- |
| Màn hình trắng | Sai cấu hình TouchGFX/LCD | Tạo project theo Board Selector, kiểm tra LTDC/FMC/SDRAM |
| Không thấy MAX30102 | Sai dây, sai nguồn, sai địa chỉ | Kiểm tra PB8/PB9, 3.3V, GND, đọc PART_ID |
| Không thấy RTC | RTC chưa cấp nguồn hoặc kéo I2C sai mức | Kiểm tra VCC, GND, địa chỉ 0x68, pin cúc áo |
| Text không cập nhật | Sai tên widget/buffer | Sửa Screen1View.cpp theo tên trong Designer |
| SpO₂ nhảy mạnh | Ngón tay cử động, ánh sáng nhiễu, chưa hiệu chuẩn | Giữ tay yên, che cảm biến, tăng cửa sổ lọc |
| Build lỗi C/C++ | Include C header trong TouchGFX C++ sai | Dùng extern "C" khi include health_monitor.h |

# 9. Nội dung cần chụp ảnh đưa vào báo cáo

Ảnh 3 module: kit STM32F429ZIT6, MAX30102, Tiny RTC.

Ảnh đấu nối I2C thực tế.

Ảnh cấu hình CubeMX I2C1 PB8/PB9.

Ảnh giao diện TouchGFX Designer.

Ảnh màn hình LCD khi Place finger.

Ảnh màn hình LCD khi đo ra BPM/SpO₂.

Ảnh bảng test so sánh nếu có máy đo chuẩn.

# 10. Câu nói giải thích khi bảo vệ

Hệ thống sử dụng STM32F429ZIT6 làm bộ xử lý trung tâm. MAX30102 đo tín hiệu quang Red/IR, Tiny RTC cung cấp thời gian thực. STM32 đọc cả hai module qua I2C, xử lý tín hiệu PPG để tính BPM và SpO₂, sau đó hiển thị kết quả lên màn LCD bằng TouchGFX. Do không dùng LED/OLED, cảnh báo và trạng thái đo được thể hiện trực tiếp trên giao diện TouchGFX.