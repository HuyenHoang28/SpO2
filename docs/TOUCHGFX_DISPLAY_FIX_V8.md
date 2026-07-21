# Hướng dẫn build và kiểm tra TouchGFX — v8

## Build sạch

1. Giải nén project vào **một thư mục mới**.
2. Import thư mục `SpO_2/STM32CubeIDE` bằng `Existing Projects into Workspace`.
3. Đóng CubeIDE và xóa `SpO_2/STM32CubeIDE/Debug` nếu IDE đã tạo thư mục này.
4. Mở lại CubeIDE, chọn `Project > Clean...`.
5. Build project `STM32F429I_DISCO_REV_D01`.
6. Flash và nhấn RESET trên kit.

Không dùng `Generate Code` từ file IOC trong bản sửa này vì thao tác đó có thể ghi
đè target HAL, linker script và cấu hình pin SDRAM/LCD đã được hiệu chỉnh.

## Kết quả mong đợi

Sau reset, Screen1 phải xuất hiện trực tiếp. LD3 xanh nhấp nháy cho biết GUI task
đang chạy. Sau khi khung đầu tiên hợp lệ, task cảm biến mới khởi tạo MAX30102 và RTC.

## Cách đọc lỗi trên LCD

- **Cam:** GUI task chưa chạy. Kiểm tra task creation, heap và scheduler.
- **Vàng:** GUI task có chạy nhưng TouchGFX không render marker. Kiểm tra đúng project
  v8, linker script và việc xóa Debug cũ.
- **Đỏ:** `Error_Handler()` đã được gọi. Debug và xem call stack tại Error_Handler.
- **Tím:** HardFault. Debug và xem thanh ghi fault `CFSR/HFSR/BFAR/MMFAR`.
- **Vẫn xanh than và LD3 không nhấp nháy:** firmware v8 chưa thực sự được flash hoặc
  object cũ vẫn đang được sử dụng.

## Kiểm tra map file sau build

Mở:

```text
STM32CubeIDE/Debug/STM32F429I_DISCO_REV_D01.map
```

Tìm `TouchGFX_Framebuffer`. Kết quả đúng phải tương đương:

```text
TouchGFX_Framebuffer  0xD0000000  0x25800
```

Chỉ được có **một** entry framebuffer. Không được còn `0xBB800`,
`animationStorage`, hoặc framebuffer bắt đầu tại `0xD0025800`.

## Kiểm tra file binary đang dùng

Trong console build phải thấy các file sau được compile từ thư mục project v8:

```text
TouchGFX/target/TouchGFXHAL.cpp
TouchGFX/target/generated/TouchGFXGeneratedHAL.cpp
TouchGFX/target/generated/TouchGFXConfiguration.cpp
TouchGFX/target/generated/STM32DMA.cpp
```

Nếu console trỏ sang thư mục bản v6/v7 hoặc workspace cũ, xóa project khỏi workspace
(không xóa nội dung trên ổ đĩa), rồi import lại đúng thư mục v8.
