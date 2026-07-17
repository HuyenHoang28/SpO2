# Cấu hình STM32CubeMX/CubeIDE cho project STM32F429ZIT6 + MAX30102 + Tiny RTC + TouchGFX

## 1. Chọn MCU/Board
- MCU: STM32F429ZIT6 hoặc chọn đúng board kit STM32F429ZIT6 đang dùng.
- Nếu là STM32F429I-DISC1/Discovery: nên tạo project theo Board Selector để CubeMX tự cấu hình LCD, SDRAM, LTDC, DMA2D.

## 2. Clock
- Dùng cấu hình clock mặc định của board trước để tránh lỗi.
- Với STM32F429, hệ thống có thể chạy đến 180 MHz, nhưng khi mới làm chỉ cần cấu hình theo CubeMX board template.

## 3. Bật TouchGFX
- Middleware: Graphics -> TouchGFX = Enable.
- Bật CRC, DMA2D, LTDC, FMC/SDRAM nếu board LCD cần.
- Generate code với Toolchain: STM32CubeIDE.

## 4. Bật I2C1 cho MAX30102 và Tiny RTC
- I2C1 Mode: I2C.
- PB8 = I2C1_SCL.
- PB9 = I2C1_SDA.
- Speed: 100 kHz khi mới test. Sau ổn định có thể nâng 400 kHz.
- Pull-up: module thường có sẵn, nhưng phải đảm bảo không kéo lên 5V.

## 5. UART debug, tùy chọn
- Có thể bật USART3 115200 để in raw Red/IR, BPM, SpO2 khi debug.
- Nếu không cần debug thì bỏ UART.

## 6. Copy code
- Copy Core/Inc/*.h vào Core/Inc của project.
- Copy Core/Src/*.c vào Core/Src của project, trừ main_integration_example.c chỉ để tham khảo.
- Copy các file TouchGFX vào đúng thư mục gui/include và gui/src sau khi đã tạo màn hình trên TouchGFX Designer.
