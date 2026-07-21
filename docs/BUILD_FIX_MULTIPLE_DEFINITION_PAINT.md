# Fix multiple definition: TouchGFX paint functions

## Lỗi

Linker báo các hàm sau được định nghĩa đồng thời trong `TouchGFXHAL.o` và `STM32DMA.o`:

- `touchgfx::paint::setL8Palette`
- `touchgfx::paint::tearDown`
- `touchgfx::paint::flushLine`
- `touchgfx::paint::invalidateTextureCache`

## Nguyên nhân

`PaintRGB565Impl.hpp` tự động include `PaintImpl.hpp`. Nếu implementation header này được include trong `TouchGFXHAL.cpp`, trong khi `STM32DMA.cpp` của target cũng định nghĩa các hàm paint chung, linker sẽ nhận hai bản định nghĩa.

## Cách sửa trong bản v4

- Không include `PaintImpl.hpp` hoặc `PaintRGB565Impl.hpp` trong `TouchGFXHAL.cpp`.
- Không include implementation header trực tiếp trong `STM32DMA.cpp`.
- Tích hợp phần triển khai RGB565 vào cùng `STM32DMA.cpp` với phần DMA2D.
- `setL8Palette()` cập nhật cả palette DMA2D và palette của software RGB565 painter.

Như vậy mỗi hàm chỉ có đúng một định nghĩa trong toàn project.

## Build lại

1. Đóng STM32CubeIDE.
2. Xóa thư mục `STM32CubeIDE/Debug` nếu còn tồn tại.
3. Mở lại project.
4. Chọn `Project > Clean`.
5. Chọn `Project > Build Project`.
