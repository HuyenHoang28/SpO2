# Sửa lỗi STM32DMA.cpp / RGB565

Project dùng `USE_BPP=16`, vì vậy TouchGFX cần các hàm trong namespace `touchgfx::paint::rgb565`.

Bản sửa biên dịch `PaintRGB565Impl.hpp` đúng một lần trong:

`TouchGFX/target/generated/STM32DMA.cpp`

Đồng thời đã xóa include tương ứng khỏi `TouchGFX/target/TouchGFXHAL.cpp` để tránh lỗi multiple definition.

Các lỗi DMA2D khác đã sửa:

- Địa chỉ CLUT dùng `palette->data`/`L8CLUT->data`, không dùng địa chỉ của trường con trỏ.
- Mask alpha foreground dùng `DMA2D_FGPFCCR_ALPHA`.
- Chuyển đổi RGB888/RGB565 không alpha dùng `DMA2D_M2M_PFC`.
- Địa chỉ con trỏ được chuyển qua `uintptr_t`.

Sau khi thay file, xóa thư mục `Debug`, chọn **Project > Clean**, rồi **Build Project**.
