# Sửa lỗi màn hình đen

Các lỗi đã được sửa:

1. PB6 từng bị cấu hình thành I2C1_SCL, xung đột với FMC_SDNE1 của SDRAM.
2. FMC từng cấu hình sai BANK1, clock SDRAM bị disable và timing đều bằng 16.
3. Chuỗi lệnh khởi tạo SDRAM chưa từng được gọi.
4. GPIO FMC dùng PC2/PC3 sai board, trong khi SDRAM thực dùng PB5/PB6.
5. Thiếu PE0/PE1 cho byte lane NBL0/NBL1.
6. LTDC timing không đúng ILI9341.
7. Layer LTDC có alpha bằng 0 nên hoàn toàn trong suốt.
8. PC2 chưa được cấu hình GPIO output cho LCD chip-select.
9. ILI9341 chỉ được gửi lệnh Display On mà chưa chạy chuỗi init đầy đủ.
10. Thiếu LTDC_IRQHandler nên TouchGFX chờ VSYNC vô hạn.

Firmware v5 chuyển MAX30102 và RTC sang I2C3 PA8/PC9, khôi phục SDRAM bank 2 và thêm ngắt LTDC.
