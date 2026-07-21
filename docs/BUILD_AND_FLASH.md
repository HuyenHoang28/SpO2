# Build và nạp chương trình

## STM32CubeIDE

1. Import thư mục `STM32CubeIDE` bằng **Existing Projects into Workspace**.
2. Đảm bảo cấu hình đang là `Debug`.
3. Chọn **Project → Clean**.
4. Chọn **Build Project**.
5. Cắm cổng USB ST-LINK của kit.
6. Chọn **Run As → STM32 C/C++ Application**.

## Khi CubeIDE báo thiếu file mới

Project hoàn chỉnh đã có linked resources cho bốn file C mới. Nếu workspace cũ
đang cache cấu trúc trước đó, đóng project, xóa project khỏi workspace nhưng
không xóa nội dung trên ổ đĩa, rồi import lại thư mục `STM32CubeIDE`.

## TouchGFX Designer

Mở `TouchGFX/SpO_2.touchgfx`. Khi Generate Code, giữ Color Depth là `16 bpp`.
Sau khi generate, build lại trong CubeIDE.

## Thiết lập giờ RTC lần đầu

Trong `Core/Src/main.c`:

```c
#define SPO2_SET_RTC_ON_BOOT 1U
```

Sửa cấu trúc `initial_time`, build và nạp một lần. Sau đó đổi lại:

```c
#define SPO2_SET_RTC_ON_BOOT 0U
```

Việc này tránh ghi đè thời gian của RTC mỗi lần khởi động.
