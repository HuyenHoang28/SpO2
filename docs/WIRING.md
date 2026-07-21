# Đấu nối phần cứng đã sửa cho STM32F429I-DISCO

## Bus I²C3 dùng chung

PB6 **không được dùng cho I²C** vì đây là chân `FMC_SDNE1` của SDRAM trên kit. Framebuffer TouchGFX nằm trong SDRAM tại `0xD0000000`; dùng PB6 làm SCL sẽ làm màn hình đen.

| Tín hiệu | STM32F429I-DISCO | MAX30102 | Tiny RTC |
|---|---|---|---|
| SCL | PA8 / I2C3_SCL | SCL | SCL |
| SDA | PC9 / I2C3_SDA | SDA | SDA |
| Nguồn | 3V | VIN/VCC | VCC |
| Mass | GND | GND | GND |

I2C3 được chia sẻ với bộ điều khiển cảm ứng STMPE811 trên kit. Địa chỉ không xung đột: MAX30102 `0x57`, RTC `0x68`, STMPE811 dùng địa chỉ riêng. Firmware dùng mutex để tránh hai task truy cập I²C3 cùng lúc.

## Các chân bắt buộc giữ nguyên

- PB5: `FMC_SDCKE1`.
- PB6: `FMC_SDNE1`.
- PB8/PB9: dữ liệu xanh LCD `LTDC_B6/B7`.
- PC2: chip-select SPI của ILI9341.
- PA8/PC9: bus I2C3 dùng chung.
