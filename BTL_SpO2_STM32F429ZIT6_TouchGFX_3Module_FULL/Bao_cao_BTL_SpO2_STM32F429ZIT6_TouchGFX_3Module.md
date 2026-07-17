**TRƯỜNG ĐẠI HỌC/VIỆN: ........................................

****KHOA/VIỆN: ...................................................

****BÁO CÁO BÀI TẬP LỚN

****MÔN: HỆ THỐNG NHÚNG

****ĐỀ TÀI

****THIẾT KẾ HỆ THỐNG ĐO NHỊP TIM VÀ SpO₂ SỬ DỤNG STM32F429ZIT6, MAX30102, TINY RTC VÀ TOUCHGFX

**

| **Giảng viên hướng dẫn** | ........................................ |
| --- | --- |
| **Nhóm thực hiện** | ........................................ |
| **Lớp** | ........................................ |
| **Học kỳ/Năm học** | ........................................ |
| **Ngày hoàn thành** | 24/06/2026 |

# DANH SÁCH THÀNH VIÊN

| **STT** | **Họ và tên** | **Mã sinh viên** | **Nhiệm vụ** |
| --- | --- | --- | --- |
| 1 | ................................ | ................ | Thiết kế phần cứng, cấu hình CubeMX |
| 2 | ................................ | ................ | Lập trình MAX30102, thuật toán SpO₂ |
| 3 | ................................ | ................ | TouchGFX, giao diện và kiểm thử |
| 4 | ................................ | ................ | Báo cáo, trình bày, tổng hợp |

# MỤC LỤC

LỜI NÓI ĐẦU

CHƯƠNG 1. MỞ ĐẦU

1.1. Đặt vấn đề

1.2. Mục tiêu đề tài

1.3. Đối tượng và phạm vi nghiên cứu

1.4. Phương pháp nghiên cứu

1.5. Ý nghĩa đề tài

CHƯƠNG 2. CƠ SỞ LÝ THUYẾT

2.1. Hệ thống nhúng và bài toán y sinh

2.2. Vi điều khiển STM32F429ZIT6

2.3. Cảm biến MAX30102

2.4. Tiny RTC DS3231/DS1307

2.5. TouchGFX

2.6. I2C

2.7. Nguyên lý PPG và tính SpO₂

CHƯƠNG 3. PHÂN TÍCH VÀ THIẾT KẾ HỆ THỐNG

3.1. Yêu cầu hệ thống

3.2. Sơ đồ khối

3.3. Thiết kế phần cứng

3.4. Thiết kế phần mềm

3.5. Thiết kế giao diện TouchGFX

CHƯƠNG 4. TRIỂN KHAI PROJECT

4.1. Danh sách linh kiện

4.2. Đấu nối

4.3. Cấu hình STM32CubeMX

4.4. Tạo giao diện TouchGFX

4.5. Tích hợp code

4.6. Kiểm thử từng module

CHƯƠNG 5. KẾT QUẢ VÀ ĐÁNH GIÁ

KẾT LUẬN

TÀI LIỆU THAM KHẢO

PHỤ LỤC CODE VÀ CHECKLIST

# LỜI NÓI ĐẦU

Hệ thống nhúng là lĩnh vực kết hợp giữa phần cứng, phần mềm và môi trường vật lý. Khác với lập trình ứng dụng thông thường, lập trình nhúng yêu cầu người thiết kế quan tâm đồng thời đến tài nguyên xử lý, bộ nhớ, ngoại vi, giao thức truyền thông, độ ổn định và tính thời gian thực.

Trong đồ án này, nhóm xây dựng một hệ thống đo nhịp tim và nồng độ oxy trong máu SpO₂ dựa trên kit STM32F429ZIT6, cảm biến quang học MAX30102, module thời gian thực Tiny RTC và giao diện đồ họa TouchGFX. Hệ thống không sử dụng OLED, không sử dụng LED cảnh báo rời và không sử dụng ESP32/Blynk. Toàn bộ quá trình đọc dữ liệu, xử lý tín hiệu và hiển thị kết quả được thực hiện trực tiếp trên STM32.

Đồ án có mục tiêu học tập và mô phỏng kỹ thuật. Kết quả đo SpO₂/BPM trong báo cáo chỉ phục vụ minh họa nguyên lý, không dùng thay thế thiết bị y tế đã được kiểm định.

# CHƯƠNG 1. MỞ ĐẦU

## 1.1. Đặt vấn đề

Nhịp tim và SpO₂ là hai chỉ số sinh tồn quan trọng. Nhịp tim phản ánh hoạt động co bóp của tim, còn SpO₂ phản ánh mức độ bão hòa oxy trong máu ngoại vi. Trong các thiết bị đo không xâm lấn, cảm biến quang học được sử dụng rộng rãi vì có thể đo trên đầu ngón tay mà không cần lấy máu.

MAX30102 là cảm biến phù hợp cho đồ án hệ thống nhúng vì nó tích hợp LED đỏ, LED hồng ngoại, photodiode, ADC và FIFO. STM32F429ZIT6 có năng lực xử lý cao, đủ để đọc dữ liệu cảm biến, lọc nhiễu, tính toán BPM/SpO₂ và chạy giao diện TouchGFX trên màn hình LCD/TFT của kit.

Việc bổ sung Tiny RTC giúp hệ thống hiển thị thời gian đo thực tế. Nhờ đó, màn hình TouchGFX không chỉ hiển thị kết quả BPM/SpO₂ mà còn hiển thị ngày giờ đo, tăng tính hoàn chỉnh của sản phẩm.

## 1.2. Mục tiêu đề tài

Thiết kế được mô hình hệ thống gồm 3 module chính: kit STM32F429ZIT6, cảm biến MAX30102 và Tiny RTC.

Cấu hình được I2C để STM32 giao tiếp đồng thời với MAX30102 và Tiny RTC trên cùng một bus.

Đọc được dữ liệu Red/IR thô từ FIFO của MAX30102, nhận biết trạng thái đặt ngón tay.

Triển khai thuật toán xử lý PPG cơ bản: lọc nhiễu, tách AC/DC, phát hiện đỉnh, tính BPM và ước lượng SpO₂.

Đọc được thời gian thực từ Tiny RTC và hiển thị giờ/ngày trên giao diện.

Thiết kế giao diện TouchGFX hiển thị SpO₂, BPM, thời gian đo và trạng thái hệ thống.

Viết báo cáo đầy đủ gồm lý thuyết, thiết kế, triển khai, kiểm thử và đánh giá.

## 1.3. Đối tượng và phạm vi nghiên cứu

Đối tượng nghiên cứu gồm vi điều khiển STM32F429ZIT6, giao thức I2C, cảm biến MAX30102, module Tiny RTC DS3231/DS1307, TouchGFX và thuật toán xử lý tín hiệu PPG. Phạm vi đồ án dừng ở mức prototype học tập, đo trên ngón tay trong điều kiện tĩnh, hiển thị kết quả trên LCD của kit, không xây dựng thiết bị y tế thương mại.

## 1.4. Phương pháp nghiên cứu

Nghiên cứu tài liệu kỹ thuật của STM32F429ZIT6, MAX30102, Tiny RTC và TouchGFX.

Thiết kế từ trên xuống: chia hệ thống thành khối cảm biến, khối thời gian, khối xử lý và khối giao diện.

Triển khai theo module: kiểm tra I2C, đọc PART_ID MAX30102, đọc RTC, xử lý thuật toán, sau đó mới tích hợp TouchGFX.

Kiểm thử thực nghiệm bằng cách so sánh kết quả đo với máy đo SpO₂ thương mại và quan sát độ ổn định của giao diện.

## 1.5. Ý nghĩa đề tài

Đề tài giúp sinh viên thực hành toàn bộ quy trình phát triển một hệ thống nhúng: lựa chọn phần cứng, cấu hình ngoại vi, đọc cảm biến qua I2C, xử lý tín hiệu thời gian thực, thiết kế giao diện GUI và kiểm thử hệ thống. Việc dùng TouchGFX làm giao diện cũng giúp đồ án có tính trực quan và chuyên nghiệp hơn so với hiển thị text đơn giản trên OLED.

# CHƯƠNG 2. CƠ SỞ LÝ THUYẾT

## 2.1. Hệ thống nhúng trong bài toán đo chỉ số y sinh

Hệ thống nhúng là hệ thống máy tính chuyên dụng được tích hợp vào thiết bị để thực hiện một nhóm chức năng cụ thể. Trong bài toán đo SpO₂, hệ thống phải làm việc với dữ liệu cảm biến liên tục, xử lý theo thời gian thực mềm và phản hồi kết quả lên giao diện người dùng.

Đặc điểm quan trọng của đồ án là sự kết hợp giữa phần cứng và phần mềm. Phần cứng gồm cảm biến PPG và RTC, phần mềm gồm driver I2C, thuật toán DSP và giao diện TouchGFX. Nếu một trong các khối này hoạt động không ổn định, kết quả hiển thị sẽ sai hoặc không cập nhật.

## 2.2. Vi điều khiển STM32F429ZIT6

STM32F429ZIT6 thuộc dòng STM32F4, sử dụng lõi ARM Cortex-M4 có FPU/DSP. Đặc điểm này phù hợp với bài toán xử lý tín hiệu PPG vì thuật toán cần tính trung bình, biên độ AC/DC, phát hiện đỉnh và cập nhật giao diện đồ họa.

| **Thành phần** | **Vai trò trong đồ án** |
| --- | --- |
| I2C1 | Giao tiếp với MAX30102 và Tiny RTC |
| Timer/HAL_GetTick | Tạo chu kỳ xử lý 10 ms, tương đương 100 Hz |
| DMA2D/LTDC/FMC/SDRAM | Phục vụ hiển thị TouchGFX tùy board |
| CRC | Yêu cầu thường gặp khi bật TouchGFX |
| Flash/RAM | Lưu chương trình, buffer tín hiệu và framebuffer GUI |

## 2.3. Cảm biến MAX30102

MAX30102 là cảm biến quang học dùng cho đo nhịp tim và SpO₂. Cảm biến phát ánh sáng đỏ và hồng ngoại vào mô ngón tay, sau đó photodiode nhận ánh sáng phản xạ. Khi tim co bóp, thể tích máu trong mao mạch thay đổi làm tín hiệu phản xạ dao động theo nhịp tim.

| **Khối trong MAX30102** | **Chức năng** |
| --- | --- |
| Red LED | Nguồn sáng đỏ, nhạy với hemoglobin khử oxy |
| IR LED | Nguồn sáng hồng ngoại, nhạy với hemoglobin bão hòa oxy |
| Photodiode | Thu ánh sáng phản xạ từ ngón tay |
| ADC | Chuyển tín hiệu quang thành dữ liệu số |
| FIFO | Lưu mẫu Red/IR để STM32 đọc theo khối |
| I2C interface | Cho phép cấu hình thanh ghi và đọc dữ liệu |

## 2.4. Tiny RTC DS3231/DS1307

Tiny RTC là module thời gian thực, địa chỉ I2C thường là 0x68. Module này không tham gia đo SpO₂ mà cung cấp giờ, phút, giây, ngày, tháng, năm để hệ thống hiển thị thời gian đo. Nếu có thể chọn, DS3231 nên được ưu tiên vì độ chính xác cao hơn DS1307 và có thể hoạt động tốt ở mức 3.3V.

Lưu ý phần cứng quan trọng: STM32F429ZIT6 dùng logic 3.3V. Một số module Tiny RTC DS1307 có điện trở kéo lên SDA/SCL về 5V. Khi dùng với STM32, cần đảm bảo bus I2C không bị kéo lên 5V để tránh làm hỏng chân MCU.

## 2.5. TouchGFX

TouchGFX là framework đồ họa cho STM32, cho phép thiết kế giao diện bằng TouchGFX Designer và tích hợp với STM32CubeIDE. Trong đồ án này, TouchGFX thay thế OLED và LED: mọi thông tin BPM, SpO₂, thời gian và cảnh báo đều hiển thị trên màn LCD/TFT của kit.

Luồng dữ liệu trong TouchGFX nên theo mô hình Model - Presenter - View. Core C code cập nhật dữ liệu đo vào HealthMonitor. Model đọc dữ liệu này theo chu kỳ, Presenter truyền xuống View, View cập nhật các TextArea trên màn hình.

## 2.6. Giao thức I2C

I2C là giao thức nối tiếp đồng bộ dùng hai dây SDA và SCL. Trong hệ thống này, STM32 đóng vai trò Master, còn MAX30102 và Tiny RTC là Slave. Hai thiết bị có địa chỉ khác nhau nên có thể dùng chung một bus I2C.

| **Thiết bị** | **Địa chỉ I2C thường gặp** | **Chức năng** |
| --- | --- | --- |
| MAX30102 | 0x57 | Đọc FIFO Red/IR, cấu hình chế độ SpO₂ |
| Tiny RTC | 0x68 | Đọc/ghi thời gian thực |
| STM32F429ZIT6 | Master | Sinh xung SCL, gửi địa chỉ, đọc/ghi thanh ghi |

## 2.7. Nguyên lý PPG và tính toán SpO₂

PPG là phương pháp đo quang học sự thay đổi thể tích máu trong mô. Tín hiệu PPG gồm thành phần DC và AC. Thành phần DC do mô, xương, tĩnh mạch và ánh sáng nền gây ra; thành phần AC chủ yếu do máu động mạch thay đổi theo nhịp tim. Nhịp tim được ước lượng bằng cách phát hiện các đỉnh trong tín hiệu IR.

SpO₂ được ước lượng bằng tỉ số R = (AC_red/DC_red) / (AC_ir/DC_ir). Sau khi tính R, hệ thống dùng công thức thực nghiệm hoặc bảng tra để suy ra SpO₂. Trong đồ án học tập, có thể dùng công thức SpO₂ = 110 - 25R để demo; khi làm sản phẩm thật cần hiệu chuẩn bằng dữ liệu y tế chuẩn.

# CHƯƠNG 3. PHÂN TÍCH VÀ THIẾT KẾ HỆ THỐNG

## 3.1. Yêu cầu hệ thống

| **Nhóm yêu cầu** | **Mô tả** |
| --- | --- |
| Chức năng đo | Đọc Red/IR từ MAX30102 và tính BPM/SpO₂ |
| Chức năng thời gian | Đọc ngày giờ từ Tiny RTC |
| Chức năng hiển thị | Hiển thị kết quả trên TouchGFX, không dùng OLED/LED |
| Chức năng trạng thái | Thông báo Place finger, Measuring, Normal, Low SpO₂ Warning |
| Độ ổn định | Không treo khi cảm biến chưa đặt tay hoặc RTC lỗi |
| An toàn | Không kéo I2C lên 5V khi nối với STM32 |

## 3.2. Sơ đồ khối tổng quan

Hình 3.1. Sơ đồ khối hệ thống gồm STM32F429ZIT6, MAX30102, Tiny RTC và TouchGFX.

Luồng dữ liệu bắt đầu từ MAX30102. Cảm biến lấy mẫu Red/IR và lưu vào FIFO. STM32 đọc FIFO qua I2C, xử lý tín hiệu để tính BPM/SpO₂, đồng thời đọc thời gian từ Tiny RTC. Kết quả được truyền vào Model của TouchGFX để hiển thị trên LCD.

## 3.3. Thiết kế phần cứng

Hình 3.2. Sơ đồ đấu nối I2C giữa STM32F429ZIT6, MAX30102 và Tiny RTC.

| **Chân STM32F429ZIT6** | **MAX30102** | **Tiny RTC** | **Ghi chú** |
| --- | --- | --- | --- |
| 3.3V | VCC/VIN | VCC | Ưu tiên cấp 3.3V cho toàn bus I2C |
| GND | GND | GND | Nối chung mass |
| PB8 - I2C1_SCL | SCL | SCL | Đường clock I2C |
| PB9 - I2C1_SDA | SDA | SDA | Đường data I2C |

## 3.4. Thiết kế phần mềm

Hình 3.3. Lưu đồ xử lý phần mềm.

| **Module phần mềm** | **File chính** | **Nhiệm vụ** |
| --- | --- | --- |
| Driver MAX30102 | max30102.c/h | Khởi tạo cảm biến, đọc PART_ID, đọc FIFO Red/IR |
| Driver Tiny RTC | tiny_rtc.c/h | Đọc/ghi thời gian ở địa chỉ I2C 0x68 |
| Thuật toán SpO₂ | spo2_algorithm.c/h | Tính BPM và SpO₂ từ dữ liệu Red/IR |
| HealthMonitor | health_monitor.c/h | Ghép cảm biến, RTC và thuật toán thành dữ liệu dùng cho GUI |
| TouchGFX GUI | Model/Presenter/View | Hiển thị kết quả lên LCD |

## 3.5. Thiết kế giao diện TouchGFX

Hình 3.4. Giao diện TouchGFX đề xuất.

Giao diện gồm bốn vùng chính: vùng SpO₂, vùng BPM, vùng thời gian/ngày đo và vùng trạng thái. Nếu chưa đặt tay, trạng thái hiển thị Place finger; nếu đang thu dữ liệu nhưng chưa đủ ổn định, hiển thị Measuring; nếu SpO₂ thấp, hiển thị Low SpO₂ Warning.

| **Widget TouchGFX** | **Tên gợi ý** | **Nội dung hiển thị** |
| --- | --- | --- |
| TextArea | textSpo2 | 98 % hoặc -- % |
| TextArea | textBpm | 78 BPM hoặc -- BPM |
| TextArea | textTime | 14:35:20 |
| TextArea | textDate | 24/06/2026 |
| TextArea | textStatus | Place finger / Measuring / Normal / Warning |

# CHƯƠNG 4. TRIỂN KHAI PROJECT

## 4.1. Danh sách linh kiện

| **STT** | **Linh kiện** | **Số lượng** | **Dùng để làm gì** |
| --- | --- | --- | --- |
| 1 | Kit STM32F429ZIT6 có LCD/TFT | 1 | Vi điều khiển chính và chạy TouchGFX hiển thị kết quả |
| 2 | MAX30102 | 1 | Đo tín hiệu PPG Red/IR để tính BPM và SpO₂ |
| 3 | Tiny RTC DS3231 hoặc DS1307 | 1 | Cung cấp thời gian thực cho mỗi lần đo |
| 4 | Dây jumper | 1 bộ | Nối các chân I2C và nguồn |
| 5 | Breadboard hoặc dây cái-cái | 1 | Lắp thử mạch |
| 6 | Cáp USB/ST-Link tích hợp | 1 | Nạp code, cấp nguồn, debug |

## 4.2. Đấu nối phần cứng

Tắt nguồn kit trước khi đấu nối.

Nối GND của MAX30102 và Tiny RTC về GND của STM32.

Nối VCC của MAX30102 và RTC về 3.3V. Nếu module RTC bắt buộc 5V, kiểm tra điện trở kéo lên I2C và dùng chuyển mức logic nếu cần.

Nối SCL của MAX30102 và RTC chung vào PB8.

Nối SDA của MAX30102 và RTC chung vào PB9.

Cấp nguồn lại và dùng I2C scanner hoặc đọc PART_ID để kiểm tra thiết bị.

## 4.3. Cấu hình STM32CubeMX/CubeIDE

Bước này tạo nền tảng để code driver và TouchGFX hoạt động. Nên tạo project theo Board Selector nếu kit là Discovery/Nucleo có LCD để CubeMX tự cấu hình các ngoại vi đồ họa cần thiết.

| **Mục cấu hình** | **Giá trị đề xuất** |
| --- | --- |
| MCU/Board | STM32F429ZIT6 hoặc đúng board kit đang dùng |
| I2C1 | PB8=SCL, PB9=SDA, 100 kHz |
| TouchGFX | Enable trong Middleware/Graphics |
| CRC | Enable, thường cần cho TouchGFX |
| LTDC/DMA2D/FMC/SDRAM | Enable theo template board LCD |
| Toolchain | STM32CubeIDE |
| Debug | Serial Wire |

Mở STM32CubeIDE, tạo project mới.

Chọn đúng board hoặc MCU STM32F429ZIT6.

Bật I2C1 ở chế độ I2C, chọn PB8/PB9.

Bật TouchGFX và các ngoại vi LCD theo cấu hình board.

Generate code.

Build thử project rỗng để đảm bảo TouchGFX chạy trước khi thêm cảm biến.

## 4.4. Tạo giao diện TouchGFX

Trong TouchGFX Designer, tạo một màn hình Screen1. Kéo các TextArea để hiển thị SpO₂, BPM, thời gian, ngày và trạng thái. Mỗi TextArea cần bật wildcard buffer để cập nhật bằng code.

| **TextArea** | **Wildcard buffer** | **Kích thước buffer gợi ý** |
| --- | --- | --- |
| textSpo2 | textSpo2Buffer | 20 |
| textBpm | textBpmBuffer | 20 |
| textTime | textTimeBuffer | 32 |
| textDate | textDateBuffer | 32 |
| textStatus | textStatusBuffer | 40 |

## 4.5. Tích hợp code

Bộ code đi kèm báo cáo gồm các driver và module cần thiết. Sau khi CubeMX sinh project TouchGFX, copy file từ thư mục code vào đúng vị trí.

| **File/thư mục** | **Cách dùng** |
| --- | --- |
| Core/Inc/*.h | Copy vào Core/Inc |
| Core/Src/max30102.c | Copy vào Core/Src |
| Core/Src/tiny_rtc.c | Copy vào Core/Src |
| Core/Src/spo2_algorithm.c | Copy vào Core/Src |
| Core/Src/health_monitor.c | Copy vào Core/Src |
| main_integration_example.c | Chỉ đọc và copy đoạn USER CODE vào main.c thật |
| TouchGFX/gui/... | Copy vào gui/include và gui/src sau khi tạo Screen1 |

Đoạn khởi tạo cần thêm vào main.c sau khi MX_I2C1_Init() đã chạy:

#include "health_monitor.h"
extern I2C_HandleTypeDef hi2c1;

/* USER CODE BEGIN 2 */
HealthMonitor_Init(&hi2c1);
/* USER CODE END 2 */

Đoạn xử lý chu kỳ 10 ms có thể đặt trong vòng lặp chính nếu project không dùng FreeRTOS:

static uint32_t lastTick = 0;
if (HAL_GetTick() - lastTick >= 10) {
    lastTick = HAL_GetTick();
    HealthMonitor_Process10ms();
}

## 4.6. Kiểm thử từng module

| **Bước test** | **Cách làm** | **Kết quả mong đợi** |
| --- | --- | --- |
| Test I2C | Dùng HAL_I2C_IsDeviceReady hoặc I2C scanner | Thấy MAX30102 0x57 và RTC 0x68 |
| Test MAX30102 | Đọc PART_ID | Trả về 0x15 |
| Test RTC | Đọc giờ/phút/giây | Thời gian thay đổi theo giây |
| Test thuật toán | Đặt tay lên cảm biến 10-15 giây | BPM và SpO₂ dần ổn định |
| Test TouchGFX | Quan sát LCD | Giá trị và trạng thái cập nhật |
| Test cảnh báo | Mô phỏng SpO₂ thấp trong code | Status hiển thị Low SpO₂ Warning |

# CHƯƠNG 5. KẾT QUẢ VÀ ĐÁNH GIÁ

## 5.1. Kết quả dự kiến

Sau khi lắp và nạp chương trình, màn hình TouchGFX hiển thị các trường SpO₂, BPM, Time, Date và Status. Khi chưa đặt tay lên MAX30102, hệ thống hiển thị Place finger. Khi đặt tay, hệ thống chuyển sang Measuring và sau vài giây hiển thị giá trị BPM/SpO₂ ước lượng.

| **Tình huống** | **Kết quả trên TouchGFX** |
| --- | --- |
| Không có ngón tay | SpO₂: -- %, BPM: -- BPM, Status: Place finger |
| Đang lấy mẫu | Status: Measuring |
| Đo ổn định | SpO₂ khoảng 95-100%, BPM theo nhịp tim người đo |
| SpO₂ thấp hoặc mô phỏng thấp | Status: Low SpO₂ Warning |
| RTC lỗi | Vẫn đo được nhưng thời gian không cập nhật chính xác |

## 5.2. Đánh giá và sai số

Để đánh giá, nhóm nên đo cùng lúc bằng thiết bị của đồ án và máy đo SpO₂ thương mại. Ghi ít nhất 10 lần đo trong điều kiện người đo ngồi yên, ngón tay đặt ổn định. Sai số được tính bằng trị tuyệt đối giữa kết quả đồ án và thiết bị tham chiếu.

| **Lần đo** | **BPM thiết bị chuẩn** | **BPM đồ án** | **Sai số BPM** | **SpO₂ chuẩn** | **SpO₂ đồ án** | **Sai số SpO₂** |
| --- | --- | --- | --- | --- | --- | --- |
| 1 |  |  |  |  |  |  |
| 2 |  |  |  |  |  |  |
| 3 |  |  |  |  |  |  |
| 4 |  |  |  |  |  |  |
| 5 |  |  |  |  |  |  |
| 6 |  |  |  |  |  |  |
| 7 |  |  |  |  |  |  |
| 8 |  |  |  |  |  |  |
| 9 |  |  |  |  |  |  |
| 10 |  |  |  |  |  |  |

## 5.3. Hạn chế

MAX30102 cần đặt ngón tay đúng vị trí; cử động mạnh làm tín hiệu PPG nhiễu.

Công thức SpO₂ trong code là công thức thực nghiệm, chưa được hiệu chuẩn y tế.

Tiny RTC DS1307 có thể gây vấn đề mức điện áp nếu module kéo I2C lên 5V.

TouchGFX phụ thuộc cấu hình LCD của kit; nếu chọn sai board hoặc sai framebuffer, màn hình có thể không hiển thị.

# KẾT LUẬN

Đồ án đã xây dựng thiết kế hoàn chỉnh cho hệ thống đo nhịp tim và SpO₂ dùng STM32F429ZIT6, MAX30102, Tiny RTC và TouchGFX. Hệ thống sử dụng I2C để giao tiếp với cả cảm biến và RTC, xử lý tín hiệu trên STM32 và hiển thị kết quả trực tiếp trên màn hình LCD/TFT bằng TouchGFX. Thiết kế này gọn hơn so với phương án dùng OLED/LED/ESP32 vì toàn bộ hiển thị được tích hợp trong kit STM32.

Hướng phát triển tiếp theo gồm: hiệu chuẩn SpO₂ bằng dữ liệu thực nghiệm, bổ sung biểu đồ sóng PPG trên TouchGFX, lưu lịch sử đo vào Flash/SD card và cải thiện thuật toán phát hiện đỉnh để giảm nhiễu khi người dùng cử động.

# TÀI LIỆU THAM KHẢO

Tài liệu kỹ thuật STM32F429ZIT6 và STM32F4 Reference Manual.

Tài liệu datasheet MAX30102 Pulse Oximeter and Heart-Rate Sensor.

Tài liệu TouchGFX Documentation và STM32CubeIDE User Guide.

Tài liệu Tiny RTC DS3231/DS1307 module.

Báo cáo mẫu đồ án hệ thống nhúng máy đo nhịp tim và SpO₂ được cung cấp làm tài liệu tham khảo cấu trúc.

# PHỤ LỤC A. CHECKLIST NỘP BÀI

Ảnh kit STM32F429ZIT6.

Ảnh MAX30102 và Tiny RTC.

Ảnh đấu dây thực tế PB8/PB9.

Ảnh cấu hình CubeMX I2C1 và TouchGFX.

Ảnh màn hình TouchGFX khi chưa đặt tay.

Ảnh màn hình TouchGFX khi đang đo.

Ảnh so sánh với máy đo thương mại nếu có.

File code zip và báo cáo Word/PDF.

# PHỤ LỤC B. DANH SÁCH FILE CODE

| **File** | **Nội dung** |
| --- | --- |
| max30102.c/h | Driver cảm biến MAX30102 |
| tiny_rtc.c/h | Driver Tiny RTC DS3231/DS1307 |
| spo2_algorithm.c/h | Thuật toán xử lý PPG |
| health_monitor.c/h | Module quản lý dữ liệu đo |
| Model.cpp/hpp | Cầu nối dữ liệu từ Core sang TouchGFX |
| Screen1Presenter.cpp/hpp | Truyền dữ liệu sang View |
| Screen1View.cpp/hpp | Cập nhật TextArea trên màn hình |
| README_HUONG_DAN.md | Hướng dẫn build và tích hợp |

# PHỤ LỤC C. HƯỚNG DẪN LÀM CHI TIẾT TỪ ĐẦU

Phụ lục này trình bày lại toàn bộ quy trình làm project theo thứ tự thực tế. Mục tiêu là khi chưa có gì ngoài ba module phần cứng, người thực hiện vẫn có thể bắt đầu từ bước tạo project, kiểm tra từng module, sau đó tích hợp hoàn chỉnh vào TouchGFX.

## C.1. Kiểm tra đúng phần cứng trước khi lập trình

Trước khi mở CubeIDE, cần xác nhận đúng cấu hình cuối cùng của đồ án. Hệ thống không còn OLED, không còn LED rời, không còn ESP32. Màn hình hiển thị là LCD/TFT có sẵn trên kit STM32F429ZIT6 và giao diện được vẽ bằng TouchGFX.

Kit STM32F429ZIT6 phải có khả năng chạy TouchGFX và có màn hình LCD/TFT. Nếu kit chỉ là board MCU không có màn hình, cần một màn TFT tương thích; nếu không thì TouchGFX không có nơi hiển thị.

MAX30102 phải là module có chân VIN/VCC, GND, SCL, SDA. Không thay bằng Pulse Sensor analog hoặc KY-039 vì các cảm biến đó thường chỉ đo nhịp tim, không đo được SpO₂.

Tiny RTC nên là DS3231 vì chạy ổn ở 3.3V. Nếu là Tiny RTC DS1307, cần đặc biệt kiểm tra mức điện áp I2C.

Cần dây jumper, cáp USB/ST-Link và máy tính cài STM32CubeIDE, STM32CubeMX tích hợp, TouchGFX Designer.

| **Module** | **Dấu hiệu nhận biết đúng** | **Lỗi hay gặp** |
| --- | --- | --- |
| STM32F429ZIT6 kit | Có chip STM32F429ZI/ZIT6, có LCD hoặc cổng LCD | Chọn nhầm project board làm TouchGFX không chạy |
| MAX30102 | Có chữ MAX30102, giao tiếp I2C, LED đỏ/IR tích hợp | Nhầm MAX30100 hoặc cảm biến analog |
| Tiny RTC | Có chip DS3231/DS1307, pin cúc áo, chân SDA/SCL | Kéo I2C lên 5V gây rủi ro cho STM32 |

## C.2. Bước 1 - Tạo project TouchGFX chạy độc lập

Không nên đấu MAX30102/RTC ngay từ đầu. Bước đầu tiên là tạo một project TouchGFX rỗng và chạy được trên màn hình kit. Nếu màn hình chưa chạy, việc thêm cảm biến sẽ làm quá trình debug phức tạp hơn.

Mở STM32CubeIDE, chọn File -> New -> STM32 Project.

Chọn Board Selector nếu kit có tên trong danh sách, ví dụ STM32F429I-DISC1. Nếu dùng board riêng, chọn MCU STM32F429ZIT6 và tự cấu hình LCD theo tài liệu board.

Bật middleware TouchGFX. Khi TouchGFX được bật, CubeMX thường yêu cầu CRC, LTDC, DMA2D và FMC/SDRAM tùy board.

Generate code, build project và nạp xuống kit.

Nếu màn hình hiện được giao diện mặc định, chuyển sang bước tiếp theo. Nếu màn trắng, phải sửa phần LCD/TouchGFX trước.

Ảnh cần chèn vào báo cáo: màn hình TouchGFX mặc định hoặc màn hình test giao diện đầu tiên.

## C.3. Bước 2 - Thiết kế màn hình TouchGFX cho đề tài

Trong TouchGFX Designer, tạo giao diện đơn giản, rõ ràng. Không nên vẽ quá phức tạp lúc đầu vì mục tiêu chính là dữ liệu đo cập nhật ổn định. Sau khi hệ thống chạy, có thể thêm màu sắc, icon trái tim hoặc biểu đồ PPG.

| **Vùng giao diện** | **Nội dung** | **Tên widget gợi ý** |
| --- | --- | --- |
| Tiêu đề | HEALTH MONITOR | textTitle |
| SpO₂ | -- % hoặc 98 % | textSpo2 |
| Nhịp tim | -- BPM hoặc 78 BPM | textBpm |
| Thời gian | 14:35:20 | textTime |
| Ngày | 24/06/2026 | textDate |
| Trạng thái | Place finger, Measuring, Normal, Low SpO2 Warning | textStatus |

Mỗi TextArea cần bật wildcard buffer. Ví dụ textSpo2 cần textSpo2Buffer. Nếu tên buffer trong Designer khác tên trong code, project sẽ báo lỗi khi build. Khi đó có hai cách sửa: đổi tên widget trong Designer hoặc sửa code Screen1View.cpp cho đúng tên thực tế.

## C.4. Bước 3 - Cấu hình I2C1 trong CubeMX

MAX30102 và Tiny RTC đều dùng I2C nên có thể dùng chung I2C1. STM32 là master, MAX30102 và RTC là slave. Hai thiết bị có địa chỉ khác nhau nên không xung đột: MAX30102 thường là 0x57, RTC thường là 0x68.

Trong Pinout, bật I2C1.

Chọn PB8 làm I2C1_SCL và PB9 làm I2C1_SDA.

Đặt tốc độ ban đầu là 100 kHz để dễ debug. Sau khi ổn định có thể nâng lên 400 kHz.

Bật pull-up ngoài nếu module không có sẵn. Nếu module đã có điện trở kéo lên, không cần thêm.

Generate code lại sau khi cấu hình.

Lưu ý: bus I2C phải dùng mức 3.3V. Với Tiny RTC DS1307 module 5V, phải kiểm tra mạch kéo lên. Nếu SDA/SCL bị kéo lên 5V, cần tháo điện trở pull-up 5V hoặc dùng mạch chuyển mức logic.

## C.5. Bước 4 - Test I2C trước khi viết thuật toán

Sau khi đấu dây, test I2C là bước bắt buộc. Nếu không thấy thiết bị trên bus, thuật toán và TouchGFX đều không thể hoạt động. Nên debug theo thứ tự: nguồn, GND, SCL, SDA, địa chỉ I2C, sau đó mới đến thanh ghi.

uint8_t ok_max = (HAL_I2C_IsDeviceReady(&hi2c1, 0x57 << 1, 3, 100) == HAL_OK);
uint8_t ok_rtc = (HAL_I2C_IsDeviceReady(&hi2c1, 0x68 << 1, 3, 100) == HAL_OK);
// ok_max = 1 nghĩa là STM32 đã thấy MAX30102
// ok_rtc = 1 nghĩa là STM32 đã thấy Tiny RTC

Nếu ok_max = 0, kiểm tra lại MAX30102. Nếu ok_rtc = 0, kiểm tra pin cúc áo RTC, nguồn và địa chỉ. Nếu cả hai đều lỗi, khả năng cao là sai dây SDA/SCL hoặc chưa nối chung GND.

## C.6. Bước 5 - Driver MAX30102

Driver MAX30102 có nhiệm vụ cấu hình cảm biến và đọc dữ liệu thô. Khi khởi tạo, hệ thống đọc PART_ID để xác nhận đúng cảm biến. Sau đó reset FIFO, chọn chế độ SpO₂, đặt sample rate, pulse width và dòng LED.

| **Thanh ghi** | **Ý nghĩa trong project** |
| --- | --- |
| PART_ID 0xFF | Kiểm tra đúng chip MAX30102, thường trả 0x15 |
| MODE_CONFIG 0x09 | Reset/chọn chế độ SpO₂ |
| SPO2_CONFIG 0x0A | Cấu hình ADC range, sample rate, pulse width |
| LED1_PA 0x0C | Cường độ LED đỏ |
| LED2_PA 0x0D | Cường độ LED hồng ngoại |
| FIFO_DATA 0x07 | Đọc dữ liệu Red/IR 18-bit |

Khi đặt tay lên cảm biến, giá trị IR thô phải tăng rõ so với khi không đặt tay. Đây là cách kiểm tra nhanh cảm biến hoạt động.

## C.7. Bước 6 - Driver Tiny RTC

Tiny RTC lưu thời gian ở dạng BCD. Do đó khi đọc thanh ghi, code phải chuyển BCD sang số thập phân. Khi set thời gian lần đầu, code phải chuyển số thập phân sang BCD rồi ghi vào thanh ghi RTC.

// Ví dụ ý tưởng chuyển BCD:
uint8_t bcd_to_dec(uint8_t val) {
    return ((val >> 4) * 10) + (val & 0x0F);
}
uint8_t dec_to_bcd(uint8_t val) {
    return ((val / 10) << 4) | (val % 10);
}

Nếu thời gian đọc ra toàn 00 hoặc không chạy, có thể RTC chưa được set time, pin cúc áo hết hoặc bit oscillator chưa được bật tùy loại chip.

## C.8. Bước 7 - Thuật toán xử lý PPG

Dữ liệu Red/IR đọc từ MAX30102 chưa phải ngay lập tức là BPM và SpO₂. STM32 cần lưu nhiều mẫu trong buffer, lọc nhiễu, ước lượng DC/AC, tìm đỉnh tín hiệu IR để tính nhịp tim và dùng tỉ số R để ước lượng SpO₂.

Bước 1: Lưu mẫu Red/IR vào buffer vòng. Ví dụ 100 mẫu tương ứng khoảng 1 giây nếu sample rate là 100 Hz.

Bước 2: Tính DC bằng trung bình của buffer.

Bước 3: Tính AC bằng trung bình trị tuyệt đối của sai lệch so với DC hoặc bằng max-min trong cửa sổ.

Bước 4: Phát hiện đỉnh trên tín hiệu IR để đếm số nhịp.

Bước 5: Tính R = (ACred/DCred)/(ACir/DCir).

Bước 6: Ước lượng SpO₂ bằng công thức thực nghiệm hoặc bảng tra.

Trong báo cáo cần nhấn mạnh thuật toán trong đồ án chỉ là mô hình học tập. Muốn dùng y tế thật phải hiệu chuẩn bằng dữ liệu chuẩn và kiểm định nghiêm ngặt.

## C.9. Bước 8 - Ghép dữ liệu vào TouchGFX

Không nên cập nhật trực tiếp widget TouchGFX từ interrupt hoặc từ driver cảm biến. Cách an toàn là tạo một module HealthMonitor lưu dữ liệu hiện tại. TouchGFX Model đọc dữ liệu này theo chu kỳ, Presenter chuyển xuống View và View cập nhật TextArea.

| **Lớp/Module** | **Nhiệm vụ** |
| --- | --- |
| HealthMonitor | Chạy trong Core C, cập nhật BPM/SpO₂/time/status |
| Model::tick() | Đọc HealthMonitor_GetData() theo chu kỳ |
| Presenter | Gọi hàm updateHealthData() của View |
| View | Đổi nội dung TextArea và invalidate widget |

void Model::tick()
{
    static int tickDiv = 0;
    tickDiv++;
    if (tickDiv >= 12) {       // cập nhật GUI khoảng 5 lần/giây
        tickDiv = 0;
        HealthData_t data = HealthMonitor_GetData();
        modelListener->healthDataChanged(data);
    }
}

## C.10. Bước 9 - Kiểm thử tích hợp

Sau khi từng module hoạt động riêng, mới tích hợp toàn hệ thống. Khi chạy hoàn chỉnh, màn hình nên có các trạng thái rõ ràng để người bảo vệ hiểu hệ thống đang làm gì.

| **Điều kiện test** | **Hiển thị mong muốn** | **Ý nghĩa** |
| --- | --- | --- |
| Chưa đặt tay | -- %, -- BPM, Place finger | Cảm biến chưa nhận tín hiệu đủ lớn |
| Vừa đặt tay | -- %, -- BPM, Measuring | Đang thu buffer và ổn định tín hiệu |
| Đặt tay ổn định | SpO₂ và BPM có giá trị | Thuật toán đã tính được kết quả |
| SpO₂ thấp/mô phỏng thấp | Low SpO₂ Warning | Cảnh báo bằng GUI thay LED |
| RTC lỗi | Giá trị đo vẫn chạy, time không cập nhật | Hệ thống chịu lỗi một phần |

## C.11. Bước 10 - Chuẩn bị demo/bảo vệ

Chuẩn bị ảnh sơ đồ khối, ảnh đấu nối thực tế và ảnh màn hình TouchGFX.

Khi demo, đặt ngón tay yên trên MAX30102 trong 10-15 giây.

Nói rõ Tiny RTC dùng để hiển thị thời gian đo, không dùng để đo nhịp tim.

Nói rõ TouchGFX thay OLED và LED; cảnh báo được hiển thị bằng giao diện.

Nếu kết quả chưa chính xác tuyệt đối, giải thích do thuật toán chưa hiệu chuẩn và cảm biến dễ nhiễu khi cử động.

# PHỤ LỤC D. THUYẾT MINH CODE THEO FILE

Phần này giải thích vai trò của từng file trong gói code. Khi nộp, nhóm có thể đưa toàn bộ file zip kèm báo cáo. Khi trình bày, không cần đọc từng dòng code mà cần giải thích được luồng dữ liệu giữa các file.

| **File** | **Hàm quan trọng** | **Giải thích** |
| --- | --- | --- |
| max30102.c/h | MAX30102_Init, MAX30102_ReadFIFO | Khởi tạo cảm biến và đọc mỗi mẫu Red/IR 18-bit từ FIFO |
| tiny_rtc.c/h | TinyRTC_GetTime, TinyRTC_SetTime | Đọc/ghi giờ ngày tháng theo BCD từ RTC địa chỉ 0x68 |
| spo2_algorithm.c/h | SpO2Algorithm_AddSample, SpO2Algorithm_GetResult | Lưu buffer, tính DC/AC, BPM và SpO₂ |
| health_monitor.c/h | HealthMonitor_Process10ms, HealthMonitor_GetData | Gộp cảm biến, RTC và thuật toán thành dữ liệu cuối cùng cho GUI |
| Model.cpp | Model::tick | Lấy dữ liệu từ Core C sang TouchGFX theo chu kỳ |
| Screen1View.cpp | updateHealthData | Cập nhật TextArea SpO₂, BPM, Time, Date, Status |

## D.1. Luồng chạy chính trong code

main.c khởi tạo HAL, clock, I2C1 và TouchGFX.

HealthMonitor_Init(&hi2c1) gọi MAX30102_Init và TinyRTC_Init.

Mỗi 10 ms, HealthMonitor_Process10ms đọc một mẫu MAX30102 và đưa vào thuật toán.

Mỗi 1 giây, HealthMonitor đọc thời gian RTC.

TouchGFX Model::tick định kỳ đọc HealthMonitor_GetData.

Screen1View hiển thị dữ liệu lên LCD.

## D.2. Các điểm cần sửa theo project thật

Vì TouchGFX sinh code phụ thuộc tên màn hình và tên widget do người dùng đặt trong Designer, có một số điểm cần sửa nếu project thật khác mẫu:

Nếu màn hình không tên Screen1, sửa đường dẫn screen1_screen thành tên màn hình tương ứng.

Nếu TextArea không tên textSpo2/textBpm/textTime/textDate/textStatus, sửa lại trong Screen1View.cpp.

Nếu buffer size macro khác TEXTSPO2_SIZE, TEXTBPM_SIZE..., dùng đúng macro do TouchGFX sinh ra.

Nếu dùng FreeRTOS, không đặt vòng while tự xử lý như ví dụ no-OS; tạo task riêng hoặc gọi xử lý trong Model::tick với chu kỳ phù hợp.

Nếu I2C không phải hi2c1, sửa HealthMonitor_Init(&hi2c1) thành handle I2C thực tế.

## D.3. Pseudocode toàn hệ thống

Start
  HAL_Init()
  SystemClock_Config()
  MX_I2C1_Init()
  MX_TouchGFX_Init()
  HealthMonitor_Init(&hi2c1)

Loop every 10 ms:
  sample = MAX30102_ReadFIFO()
  SpO2Algorithm_AddSample(sample.red, sample.ir)
  result = SpO2Algorithm_GetResult()
  time = TinyRTC_GetTime() every 1 second
  healthData = merge(result, time, status)

TouchGFX tick:
  data = HealthMonitor_GetData()
  Presenter sends data to View
  View updates TextArea values
End

## D.4. Gợi ý mở rộng nếu muốn nâng điểm

Thêm biểu đồ sóng PPG realtime trên TouchGFX.

Thêm nút cảm ứng Start/Stop đo.

Lưu 10 kết quả đo gần nhất vào bộ nhớ Flash hoặc thẻ SD.

Thêm màn hình cài đặt thời gian RTC.

Dùng bộ lọc IIR hoặc moving average nhiều mức để tín hiệu ổn định hơn.

Thêm chế độ debug UART để vẽ raw Red/IR trên máy tính.