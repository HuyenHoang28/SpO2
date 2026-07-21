# Review chuyên sâu lỗi TouchGFX chỉ hiện nền xanh than — bản v8

## 1. Triệu chứng đã xác nhận

LCD có thể hiển thị ba dải màu ghi trực tiếp bằng CPU, nhưng sau khi vào TouchGFX
chỉ còn nền xanh than. Điều này chứng minh các khối cơ bản sau đã hoạt động:

- nguồn và đèn nền LCD;
- bộ điều khiển ILI9341;
- LTDC;
- SDRAM ngoài;
- đường dữ liệu RGB từ STM32 tới LCD.

Vì vậy lỗi không còn nằm ở dây LCD hay phần khởi tạo SDRAM cơ bản. Lỗi nằm trong
đường render/presentation của TouchGFX.

## 2. Bằng chứng từ linker map của bản cũ

Map file của firmware cũ cho thấy section framebuffer bắt đầu tại `0xD0000000`,
nhưng bên trong section có hai vùng:

```text
TouchGFX_Framebuffer  0xD0000000  0xBB800
  TouchGFXHAL.o                 0xD0000000  0x25800
  TouchGFXGeneratedHAL.o        0xD0025800  0x96000
```

Vùng đầu tiên là `animationStorage`, còn framebuffer TouchGFX thật bắt đầu từ
`0xD0025800`. Trong khi đó LTDC đã được cấu hình quét từ `0xD0000000`. TouchGFX
vẽ vào vùng khác với vùng LCD đang quét, nên người dùng chỉ thấy dữ liệu nền hoặc
một vùng nhớ chưa được vẽ đúng.

Đây là nguyên nhân có bằng chứng trực tiếp, không phải suy đoán.

## 3. Các lỗi kiến trúc bổ sung

Project đã trải qua nhiều lần vá giữa target cũ và generated/framework mới:

- target ban đầu mang cấu trúc của TouchGFX 4.19.1;
- generated code và framework hiện tại là TouchGFX 4.26.1;
- từng có cả software painter và DMA2D painter cùng được biên dịch;
- từng có double framebuffer phụ thuộc LTDC line interrupt;
- `Error_Handler()` cũ có thể quay trở lại thay vì dừng hệ thống;
- TouchGFX từng được khởi tạo trước RTOS kernel dù framework tạo object RTOS.

Các điểm này tạo một chuỗi phụ thuộc khó chẩn đoán: DMA2D phải hoàn thành, LTDC
line interrupt phải chạy, semaphore phải đúng, back buffer phải được swap và CFBAR
phải đổi chính xác. Chỉ một mắt xích hỏng cũng gây màn hình giữ nguyên màu nền.

## 4. Kiến trúc ổn định của v8

Bản v8 cố ý tối giản đường hiển thị để ưu tiên độ chắc chắn:

1. Chỉ có **một framebuffer RGB565** kích thước `240 × 320 × 2 = 153600 byte`.
2. Framebuffer được đặt cố định tại `0xD0000000` bằng linker script.
3. Linker có `ASSERT`; build sẽ thất bại nếu địa chỉ hoặc kích thước sai.
4. LTDC luôn quét đúng framebuffer duy nhất đó.
5. Dùng `touchgfx::NoDMA` và software painter RGB565 chính thức.
6. Không phụ thuộc DMA2D interrupt để render khung đầu tiên.
7. Không phụ thuộc LTDC line interrupt để gọi `tick()` hoặc swap buffer.
8. GUI task tự tạo nhịp khoảng 16 ms và gọi chuỗi:

```cpp
vSync();
backPorchExited();
frontPorchEntered();
```

9. RTOS kernel được khởi tạo trước `MX_TouchGFX_Init()`.
10. MAX30102 và RTC chỉ bắt đầu sau khi TouchGFX đã tạo được khung hợp lệ.

## 5. Kiểm thử render độc lập

Bộ generated code thực tế của project, gồm `FrontendHeap`, `Application`, screen,
font, text database và TouchGFX 4.26.1, đã được liên kết vào một chương trình test
headless. Test gọi đúng vòng `HAL::tick()` thay vì vẽ widget thủ công.

Sau 5 frame, framebuffer chứa:

```text
cyan=1200
green=7244
blue=7424
status=11142
white=421
```

Các màu này đúng với panel được tạo bởi `Screen1View`. Ảnh render có đầy đủ:

- `HEALTH MONITOR`;
- ô SpO2;
- ô BPM;
- thời gian;
- ngày;
- trạng thái `PLACE FINGER`.

Kết luận: generated screen, font, text, application transition và vòng TouchGFX
tick đều hoạt động. Lỗi của bản cũ nằm ở đường chạy target/framebuffer, không nằm
ở thiết kế Screen1.

## 6. Mã màu chẩn đoán trên kit

Bản v8 tự kiểm tra sau khi scheduler chạy:

| Hiển thị | Ý nghĩa |
|---|---|
| Giao diện HEALTH MONITOR | TouchGFX đã render thành công |
| Cam toàn màn hình | Scheduler chạy nhưng GUI task chưa vào vòng TouchGFX |
| Vàng toàn màn hình | GUI task chạy nhưng framebuffer không có marker widget |
| Đỏ toàn màn hình | Lỗi khởi tạo ngoại vi, mutex hoặc tạo task |
| Tím/magenta toàn màn hình | HardFault sau khi SDRAM đã sẵn sàng |
| LD3 xanh nhấp nháy | Vòng GUI vẫn đang chạy |

Việc báo bằng màu được thực hiện bằng ghi trực tiếp vào framebuffer sau khi GUI
task bị suspend, nên không phụ thuộc TouchGFX.

## 7. Kiểm tra kỹ thuật đã thực hiện

- Kiểm tra cú pháp các file target C/C++ quan trọng bằng Clang 17.
- Kiểm tra project chỉ link một bản của `TouchGFXHAL.cpp`,
  `TouchGFXGeneratedHAL.cpp` và `STM32DMA.cpp`.
- Kiểm tra software painter RGB565 chỉ được triển khai một lần.
- Kiểm tra linker script đặt framebuffer tại `0xD0000000`, kích thước `0x25800`.
- Chạy test render bằng vòng `HAL::tick()` thực tế và so khớp pixel marker.
- Kiểm tra framebuffer đầu tiên không còn `animationStorage` đứng trước.
- Kiểm tra TouchGFX được khởi tạo sau `osKernelInitialize()`.

## 8. Giới hạn xác nhận

Môi trường đóng gói không có bộ ARM GNU Tools 14.3.1 dành cho Windows và không thể
flash trực tiếp kit vật lý. Vì vậy không tuyên bố đã chạy firmware trên chính bo
mạch của người dùng. Tuy nhiên, cú pháp target, bố trí linker và toàn bộ vòng render
TouchGFX đã được kiểm thử riêng biệt trước khi đóng gói.
