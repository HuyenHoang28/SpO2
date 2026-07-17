# Fix màn hình chỉ hiện màu xanh than

## Nguyên nhân

`TouchGFXHAL.cpp` cũ tạo `animationStorage` trong cùng input section
`TouchGFX_Framebuffer` với mảng `frameBuf` của generated HAL. Trình linker có
thể đặt `animationStorage` tại đầu SDRAM (`0xD0000000`) và đặt `frameBuf` phía
sau. Trong khi LTDC ban đầu vẫn đọc từ `0xD0000000`, TouchGFX lại vẽ vào địa
chỉ khác, vì vậy LCD chỉ hiện nền trống.

## Sửa trong v7

1. Xóa `animationStorage` khỏi `TouchGFXHAL.cpp`.
2. Chỉ để generated HAL sở hữu section framebuffer.
3. Ép framebuffer căn hàng 32 byte.
4. Gán `LTDC_Layer1->CFBAR` trực tiếp tới `frameBuf` ngay trong
   `TouchGFXGeneratedHAL::initialize()`.
5. Chuyển `osKernelInitialize()` lên trước `MX_TouchGFX_Init()` để các đối tượng
   RTOS của TouchGFX được tạo đúng thứ tự.
6. Linker giữ section framebuffer ở đầu SDRAM và xuất symbol start/end để dễ
   kiểm tra trong map file.

## Giá trị cần thấy trong map/debug

- `__touchgfx_framebuffer_start__ = 0xD0000000`
- `frameBuf = 0xD0000000`
- `LTDC_Layer1->CFBAR = 0xD0000000` ngay sau khởi tạo

Không Generate Code lại từ file IOC cũ vì có thể ghi đè các sửa đổi phần cứng.
