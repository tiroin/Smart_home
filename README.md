# 🏠 Smart Home - ESP32

Hệ thống nhà thông minh sử dụng **ESP32**, tích hợp khóa cửa bằng mật khẩu, cảm biến môi trường và điều khiển từ xa qua **Blynk IoT**.

---

## 📋 Tính năng

| Tính năng | Mô tả |
|---|---|
| 🔐 Khóa cửa thông minh | Nhập mật khẩu qua keypad 4×4 (I2C), servo mở/đóng cửa |
| 👁️ Cảm biến PIR | Phát hiện người, tự động đóng cửa sau 3 giây |
| ⏱️ Tự động đóng | Đóng cửa sau 10 giây nếu không có người vào |
| 🌡️ DHT22 | Đọc nhiệt độ & độ ẩm mỗi 2 giây |
| 💨 MQ-2 | Đo nồng độ khí CO (ppm), cảnh báo khi vượt ngưỡng |
| 🖥️ OLED SSD1306 | Hiển thị thông số môi trường theo thời gian thực |
| 🖥️ LCD 16×2 (I2C) | Hiển thị trạng thái khóa & hướng dẫn nhập phím |
| 🔔 Buzzer | Còi cảnh báo khi nhập sai mật khẩu |
| 💡 LED | Bật/tắt bằng nút vật lý (ngắt) hoặc qua Blynk |
| 📱 Blynk IoT | Giám sát sensor & mở khóa từ xa qua điện thoại |
| 📶 Offline mode | Hoạt động bình thường khi mất kết nối WiFi |

---

## 🛒 Phần cứng cần thiết

| Linh kiện | Số lượng |
|---|---|
| ESP32 | 1 |
| Servo Motor (SG90 hoặc tương đương) | 1 |
| Cảm biến PIR HC-SR501 | 1 |
| Cảm biến khí MQ-2 | 1 |
| Cảm biến DHT22 | 1 |
| Màn hình OLED 0.96" SSD1306 (I2C) | 1 |
| Màn hình LCD 16×2 (I2C, địa chỉ 0x27) | 1 |
| Bàn phím Keypad 4×4 (I2C, PCF8574, địa chỉ 0x20) | 1 |
| Buzzer | 1 |
| LED | 1 |
| Nút nhấn | 2 |

---

## 📌 Sơ đồ chân (Pin Mapping)

| Chân ESP32 | Kết nối |
|---|---|
| GPIO 0 | DHT22 (Data) |
| GPIO 1 | Nút nhấn mở cửa (BUTTON) |
| GPIO 2 | MQ-2 (Analog out) |
| GPIO 3 | PIR HC-SR501 |
| GPIO 4 | I2C SDA (LCD + OLED + Keypad) |
| GPIO 5 | I2C SCL (LCD + OLED + Keypad) |
| GPIO 6 | Buzzer |
| GPIO 7 | LED |
| GPIO 8 | Nút nhấn LED (Interrupt) |
| GPIO 10 | Servo Motor |

> **Lưu ý:** LCD (0x27), OLED (0x3C) và Keypad PCF8574 (0x20) đều dùng chung bus I2C.

---

## 🎹 Điều khiển qua Keypad

| Phím | Chức năng |
|---|---|
| `0–9` | Nhập ký tự mật khẩu |
| `A` | Xác nhận |
| `B` | Xóa ký tự cuối |
| `C` | Đổi mật khẩu (nhập MK cũ → MK mới) |
| `D` | Reset mật khẩu (cần nhập PIN `0000`) |

> Mật khẩu mặc định sau reset: **`1234`**
> Hệ thống khóa sau **10 lần nhập sai** liên tiếp.

---

## 📱 Kết nối Blynk

| Virtual Pin | Chức năng |
|---|---|
| `V0` | Nhiệt độ (°C) |
| `V1` | Độ ẩm (%) |
| `V2` | Nồng độ CO (ppm) |
| `V3` | Nút mở khóa từ xa |
| `V4` | Bật/tắt LED từ xa |

---

## ⚠️ Cảnh báo môi trường

Hệ thống hiển thị `ALERT!` trên OLED khi:
- Nồng độ CO > **35 ppm**
- Nhiệt độ > **40°C**
- Độ ẩm > **90%**

---

## 📦 Thư viện cần cài đặt

Cài đặt qua **Arduino Library Manager**:

```
BlynkSimpleEsp32
Adafruit GFX Library
Adafruit SSD1306
DHT sensor library
Keypad_I2C
LiquidCrystal I2C
ESP32Servo
```

Và file nội bộ `Store_flash.h` (quản lý lưu mật khẩu vào Flash/EEPROM).

---

## ⚙️ Cài đặt & Nạp code

1. Cài **Arduino IDE** và thêm board ESP32.
2. Cài đầy đủ thư viện ở trên.
3. Mở file `smart_home.ino`.
4. Tạo file `config.h` (xem bên dưới) để điền thông tin cá nhân — **KHÔNG hardcode vào file `.ino`**.
5. Chọn đúng board và cổng COM, nhấn **Upload**.

---

## 🔑 Cấu hình (Quan trọng!)

> ⚠️ **Không commit thông tin nhạy cảm lên GitHub!**

Tạo file `config.h` riêng và thêm vào `.gitignore`:

```cpp
// config.h
#define BLYNK_TEMPLATE_ID   "your_template_id"
#define BLYNK_TEMPLATE_NAME "your_template_name"
#define BLYNK_AUTH_TOKEN    "your_auth_token"

const char* ssid = "your_wifi_ssid";
const char* pass = "your_wifi_password";
```

Thêm vào `.gitignore`:
```
config.h
```

---

## 📁 Cấu trúc thư mục

```
smart_home/
├── smart_home.ino      # Code chính
├── Store_flash.h       # Quản lý lưu mật khẩu vào Flash
├── config.h            # Thông tin WiFi & Blynk (KHÔNG commit)
└── README.md
```

---

## 🔄 Luồng hoạt động

```
Nhập mật khẩu (Keypad / Blynk)
        │
        ▼
   Kiểm tra MK
   ┌────┴────┐
Đúng        Sai → Buzzer → +1 lần sai → [≥10 lần → LOCKED]
   │
   ▼
Servo mở cửa
   │
   ▼
PIR chờ người vào
   ├── Phát hiện người → đóng sau 3s
   └── Không có người  → đóng sau 10s
```

---

## 👤 Tác giả

**TNVXL** — Đồ án hệ thống nhà thông minh ESP32

---

## 📄 License

MIT License
