# Project SpO2 STM32F429ZIT6 + MAX30102 + Tiny RTC + TouchGFX

## Cấu hình phần cứng cuối cùng
- STM32F429ZIT6 kit có LCD/TFT chạy TouchGFX.
- MAX30102: đo nhịp tim và SpO2.
- Tiny RTC DS3231/DS1307: lấy thời gian đo.

Không dùng OLED, không dùng LED rời, không dùng ESP32/Blynk.

## Đấu nối I2C
| STM32F429ZIT6 | MAX30102 | Tiny RTC |
|---|---|---|
| 3.3V | VCC | VCC |
| GND | GND | GND |
| PB8/I2C1_SCL | SCL | SCL |
| PB9/I2C1_SDA | SDA | SDA |

Địa chỉ I2C:
- MAX30102: 0x57.
- RTC: 0x68.

## Trình tự làm
1. Tạo project TouchGFX cho đúng kit STM32F429ZIT6.
2. Bật I2C1 PB8/PB9 trong CubeMX.
3. Tạo màn hình Screen1 trong TouchGFX Designer với các TextArea như README_TouchGFX_UI.md.
4. Generate code.
5. Copy driver MAX30102, Tiny RTC, thuật toán SpO2 và HealthMonitor vào Core.
6. Copy file Model/Presenter/View mẫu vào thư mục TouchGFX gui.
7. Trong main.c, gọi HealthMonitor_Init(&hi2c1).
8. Cứ mỗi 10ms gọi HealthMonitor_Process10ms().
9. Build, nạp code, đặt ngón tay lên MAX30102 và xem kết quả trên LCD TouchGFX.

## Lưu ý quan trọng
- Nếu Tiny RTC là DS1307 loại module 5V, không để SDA/SCL bị kéo lên 5V vì STM32 là 3.3V.
- Nếu cảm biến không nhận, dùng I2C scanner hoặc đọc PART_ID MAX30102.
- Công thức SpO2 trong code là công thức demo/học tập, không dùng thay thiết bị y tế.
