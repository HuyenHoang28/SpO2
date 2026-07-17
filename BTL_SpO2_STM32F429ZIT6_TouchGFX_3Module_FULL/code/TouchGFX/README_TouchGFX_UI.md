# Thiết kế giao diện TouchGFX

Tạo 1 màn hình tên `Screen1` trong TouchGFX Designer.

## Widget cần tạo
1. TextArea `textSpo2` với wildcard buffer `textSpo2Buffer`, size khoảng 20 ký tự.
2. TextArea `textBpm` với wildcard buffer `textBpmBuffer`, size khoảng 20 ký tự.
3. TextArea `textTime` với wildcard buffer `textTimeBuffer`, size khoảng 32 ký tự.
4. TextArea `textDate` với wildcard buffer `textDateBuffer`, size khoảng 32 ký tự.
5. TextArea `textStatus` với wildcard buffer `textStatusBuffer`, size khoảng 40 ký tự.

## Gợi ý bố cục
- Tiêu đề: HEALTH MONITOR.
- Khung trái: SpO2: xx %.
- Khung phải: BPM: xx.
- Dòng dưới: thời gian và ngày đo.
- Dòng trạng thái: Place finger / Measuring / Normal / Low SpO2 Warning.

## Luồng cập nhật
Core C code -> HealthMonitor_GetData() -> Model::tick() -> Presenter -> View -> update TextArea.
