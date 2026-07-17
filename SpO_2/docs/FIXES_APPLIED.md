# Các lỗi đã sửa trong v5

- MAX30102 và Tiny RTC chuyển sang I2C3 PA8/PC9.
- Khôi phục PB5/PB6 cho SDRAM bank 2.
- Sửa FMC bank, timing, clock, read burst và chuỗi lệnh khởi tạo SDRAM.
- Sửa GPIO NBL0/NBL1 PE0/PE1.
- Sửa timing LTDC cho ILI9341 và đặt alpha layer bằng 255.
- Cấu hình PC2 làm LCD chip-select và gọi `ili9341_Init()`.
- Bổ sung `LTDC_IRQHandler()` để TouchGFX nhận VSYNC.
- Thêm mutex cho bus I2C3 dùng chung với STMPE811.
- Thêm màn kiểm tra ba dải màu RGB trong 800 ms khi khởi động.
