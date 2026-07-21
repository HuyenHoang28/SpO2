# Sửa lỗi linker TouchGFX RGB565

## Triệu chứng

Build hoàn tất các bước biên dịch C/C++ nhưng dừng ở bước tạo file ELF với các lỗi dạng:

- `undefined reference to touchgfx::paint::rgb565::lineFromColor(...)`
- `undefined reference to touchgfx::paint::rgb565::lineFromRGB565(...)`
- `undefined reference to touchgfx::paint::rgb565::lineFromARGB8888(...)`
- `Unknown destination type (ARM/Thumb)`
- `dangerous relocation: unsupported relocation`

## Nguyên nhân

Framework và các file generated của project đang ở TouchGFX 4.26.1, trong khi file
`TouchGFX/target/TouchGFXHAL.cpp` ban đầu được tạo bởi TouchGFX Generator 4.19.1.
File target cũ không biên dịch phần triển khai software painter dành cho RGB565.
Thư viện TouchGFX vì thế gọi các hàm `lineFrom...` nhưng project không cung cấp phần định nghĩa.

## Phần đã sửa

Đã bổ sung vào vùng USER CODE của `TouchGFX/target/TouchGFXHAL.cpp`:

```cpp
#include <touchgfx/hal/PaintRGB565Impl.hpp>
```

Header này chứa phần triển khai RGB565 cần được biên dịch đúng một lần trong target application.
Không cần thay thư viện TouchGFX, không cần đổi RGB565 sang ARGB8888 và không cần hạ compiler.

## Build lại

Trong STM32CubeIDE:

1. Chọn `Project > Clean...`.
2. Chọn project `STM32F429I_DISCO_REV_D01`.
3. Nhấn `Clean`.
4. Chọn `Project > Build Project`.

Nếu workspace vẫn giữ dependency cũ, đóng CubeIDE, xóa thư mục `Debug` và `Release` bên
trong thư mục `STM32CubeIDE`, sau đó mở lại và build.
