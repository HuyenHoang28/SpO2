# MAX30102 I2C diagnostic v12

Bản v12 không giả định `PLACE FINGER` trước khi cảm biến được kiểm tra. LCD hiển thị kết quả chẩn đoán trực tiếp:

- `CHECKING SENSOR`: đang kiểm tra bus.
- `BUS OK, NO 0x57`: I2C3 hoạt động và thấy thiết bị khác trên bus, nhưng MAX30102 không ACK địa chỉ 0x57.
- `I2C BUS EMPTY`: SCL/SDA đang ở mức cao nhưng không có địa chỉ quen thuộc nào ACK.
- `SDA STUCK LOW`: SDA bị giữ ở 0 V.
- `SCL STUCK LOW`: SCL bị giữ ở 0 V.
- `BAD SENSOR ID`: thiết bị 0x57 có phản hồi nhưng thanh ghi Part ID không phải 0x15.
- `PLACE FINGER`: địa chỉ 0x57, Part ID và cấu hình MAX30102 đều thành công.

Để chẩn đoán sạch, tháo Tiny RTC và chỉ để MAX30102.
