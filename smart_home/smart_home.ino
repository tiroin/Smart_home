// ================= Config =================
#define BLYNK_TEMPLATE_ID "TMPL6lcyTmqYG"
#define BLYNK_TEMPLATE_NAME "TNVXL"
#define BLYNK_AUTH_TOKEN "nZIaZN4U2Dn24ktjxZzIDJUtgz9792DH"

#include <WiFi.h>
#include <BlynkSimpleEsp32.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <DHT.h>
#include <Keypad_I2C.h>
#include <Keypad.h>
#include <LiquidCrystal_I2C.h>
#include <ESP32Servo.h>
#include "Store_flash.h"  
#include <math.h>

//Khai báo chân
#define SERVO_PIN 10
#define PIR_PIN 3
#define BUTTON_PIN 1
#define BUZZER_PIN 6
#define I2C_SDA 4
#define I2C_SCL 5
#define DHTPIN 0
#define MQ2_PIN 2
#define EXT_BUTTON_PIN 8
#define LED_PIN   7



char auth[] = BLYNK_AUTH_TOKEN;
char ssid[] = "Quy";
char pass[] = "123443211";

// ================== LCD ==================

LiquidCrystal_I2C lcd(0x27, 16, 2);


// ===== OLED Config =====
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET   -1
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// ================== DHT ==================
#define DHTTYPE DHT22
DHT dht(DHTPIN, DHTTYPE);

// ================== MQ2 ==================
#define VCC 3.3
#define ADC_RES 4095.0
#define RL 10000.0
float R0 = 1.0;



// ===== Timer =====
BlynkTimer timer;
bool wifiConnected = false;



// --- Servo 
Servo lockServo; 


//keypad i2c
const byte ROWS = 4;
const byte COLS = 4;
char keys[ROWS][COLS] = {
  {'1','2','3','A'},
  {'4','5','6','B'},
  {'7','8','9','C'},
  {'*','0','#','D'}
};
byte rowPins[ROWS] = {0, 1, 2, 3}; 
byte colPins[COLS] = {4, 5, 6, 7};
Keypad_I2C keypad(makeKeymap(keys), rowPins, colPins, ROWS, COLS, 0x20, PCF8574);



String password;
String input = "";
const String pinReset = "0000"; // mã PIN reset

int attempts = 0;
const int maxAttempts = 10;
bool locked = false;

enum Mode { NORMAL, CHANGE_OLD, CHANGE_NEW };
Mode mode = NORMAL;

// ==== Biến debounce cho nút ====
unsigned long lastDebounceTime = 0;
unsigned long debounceDelay = 20; // ms
int lastButtonState = HIGH;
int buttonState;

// ==== PIR logic ====
bool waitForPerson = false;
bool personDetected = false;
unsigned long pirTriggerTime = 0;
unsigned long pirDelay = 3000;
unsigned long openTime = 0;           
const unsigned long autoCloseTime = 10000; 
int countdownTime = 10; 


volatile bool sampling_flag = false;
hw_timer_t *timer0 = NULL;

void IRAM_ATTR onTimer() {
  sampling_flag = true;
}

volatile bool ledOn = false;
void IRAM_ATTR handleExtButton() {
  ledOn = !ledOn;
  digitalWrite(LED_PIN, ledOn ? HIGH : LOW);
  Serial.println(">>> Button interrupt!");
}
void setup() {
  Serial.begin(115200);

  Wire.begin(I2C_SDA, I2C_SCL);
  // Khởi tạo EEPROM/Flash
  initStorage();
  password = loadPassword(); 
Serial.print("Password: "); Serial.println(password);

  // LCD
  
   lcd.init(); lcd.backlight(); lcd.setCursor(0,0); lcd.print("Nhap mat khau:");

  // Keypad
  keypad.begin();

  //DHT
  dht.begin();

  // MQ2
  pinMode(MQ2_PIN, INPUT);

  // OLED init
  if(!display.begin(SSD1306_SWITCHCAPVCC,0x3C)){ Serial.println("SSD1306 fail"); for(;;); }

  // Servo
  lockServo.attach(SERVO_PIN);
  lockServo.write(70); 

  // Nút
  pinMode(BUTTON_PIN, INPUT_PULLUP);

  // PIR
  pinMode(PIR_PIN, INPUT);

  //Loa
  pinMode(BUZZER_PIN, OUTPUT);
  digitalWrite(BUZZER_PIN, LOW);

  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.println("DHT22 + MQ2 Ready");
  display.display();

  // Timer 2s
  timer0 = timerBegin(1000000);          
  timerAttachInterrupt(timer0, &onTimer); 
  timerAlarm(timer0, 2000000, true, 0);   


  // WiFi + Blynk
 WiFi.begin(ssid, pass);
unsigned long startAttemptTime = millis();

// Thử kết nối WiFi tối đa 10 giây
while (WiFi.status() != WL_CONNECTED && millis() - startAttemptTime < 10000) {
  delay(500);
  Serial.print(".");
}

if (WiFi.status() == WL_CONNECTED) {
  Serial.println("\nWiFi connected!");
  wifiConnected = true;
  Blynk.config(auth);    
  Blynk.connect(5000);   
} else {
  Serial.println("\nWiFi not connected, running offline mode");
  wifiConnected = false;
}

  // Hiệu chuẩn MQ-2
  float rs = getMQ2Rs();
  R0 = rs / 9.8;
  Serial.printf("MQ2 done. R0 = %.2f\n", R0);

  pinMode(EXT_BUTTON_PIN, INPUT_PULLUP);  // nút nhấn kéo xuống GND
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);

  attachInterrupt(digitalPinToInterrupt(EXT_BUTTON_PIN), handleExtButton, FALLING);
}

void loop(){
  
  if(wifiConnected && Blynk.connected()) {
  Blynk.run();      
}


  // Keypad
  char key = keypad.getKey();
  if(key) handleKey(key);

  // Nút mở cửa
  int reading = digitalRead(BUTTON_PIN);
  if(reading!=lastButtonState) lastDebounceTime=millis();
  if((millis()-lastDebounceTime)>debounceDelay){
    if(reading!=buttonState){
      buttonState=reading;
      if(buttonState==LOW && !waitForPerson){
        Serial.println("Button pressed -> Mo khoa");
        lcd.clear(); lcd.setCursor(0,0); lcd.print("Mo cua bang nut");
        openLock(); resetInput();
      }
    }
  }
  lastButtonState=reading;

  // PIR
  if(waitForPerson){
    int pirState = digitalRead(PIR_PIN);
    if(pirState==HIGH && !personDetected){
      personDetected=true;
      pirTriggerTime=millis();
      Serial.println("PIR: Phat hien nguoi");
    }
    if(personDetected){
      unsigned long elapsed = millis()-pirTriggerTime;
      int remaining = 3-(elapsed/1000);
      static int lastRemaining=-1;
      if(remaining!=lastRemaining && remaining>=0){
        lcd.setCursor(0,0); lcd.print("Nguoi da vao   ");
        lcd.setCursor(0,1); lcd.print("Dong sau: "); lcd.print(remaining); lcd.print("s   ");
        lastRemaining=remaining;
      }
      if(elapsed>=3000){
        Serial.println("Dong cua sau PIR");
        lockServo.write(70);
        waitForPerson=false; personDetected=false;
        lcd.clear(); lcd.setCursor(0,0); lcd.print("Cua da dong"); resetInput();
      }
    }
  }

  // Auto close
  autoCloseIfNoPerson();

  if(sampling_flag){
    sampling_flag=false;
    sendSensor();
  }
}

// ==== Các hàm phụ ====
void resetInput() {
  input = "";
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("Nhap mat khau:");
}

void openLock() {
  lockServo.write(170); // mở cửa
  delay(3000);
  lockServo.write(70 );  // khóa lại
}

void openLock_PIR() {
  lockServo.write(170); // mở cửa
  Serial.println("Cua mo, cho PIR xac nhan...");
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("Mo cua, vui long");
  lcd.setCursor(0,1);
  lcd.print("Di vao...");

  waitForPerson = true;
  personDetected = false;
  openTime = millis();  // ghi nhận thời điểm cửa mở
  countdownTime = 10;   // reset lại bộ đếm ngược
}

void checkPassword() {
  lcd.clear();
  if (input == password) {
    lcd.setCursor(0,0);
    lcd.print("Mat Khau Dung");
    Serial.println("OPEN");
    openLock_PIR();
    attempts = 0;
  } else {
    lcd.setCursor(0,0);
    lcd.print("Sai Mat Khau");
    Serial.println("Wrong password -> Buzzer alarm");
    beepError();   
    attempts++;
    if (attempts >= maxAttempts) {
      locked = true;
      lcd.clear();
      lcd.setCursor(0,0);
      lcd.print("LOCKED!");
      lcd.setCursor(0,1);
      lcd.print("Quet the tu");
    }
  }
  delay(2000);
  if (!locked) resetInput();
  input = "";
}

// ==== Xử lý phím từ keypad ====
void handleKey(char key) {
  if (locked) return;

  if (key == 'A') { // Xác nhận
    if (mode == NORMAL) {
      checkPassword();
    } else if (mode == CHANGE_OLD) {
      if (input == password) {
        lcd.clear();
        lcd.setCursor(0,0);
        lcd.print("Nhap MK moi:");
        input = "";
        mode = CHANGE_NEW;
      } else {
        lcd.clear();
        lcd.setCursor(0,0);
        lcd.print("Sai MK cu!");
        delay(2000);
        mode = NORMAL;
        resetInput();
      }
    } else if (mode == CHANGE_NEW) {
      if (isValidPassword(input)) {
        password = input;
        savePassword(password);    
        Serial.print("Da luu password moi: ");
        Serial.println(password);
        lcd.clear();
        lcd.setCursor(0,0);
        lcd.print("MK da doi!");
        delay(2000);
        mode = NORMAL;
        resetInput();
      } else {
        lcd.clear();
        lcd.setCursor(0,0);
        lcd.print("MK khong hop le!");
        delay(2000);
        mode = NORMAL;
        resetInput();
      }
    }
  }
  else if (key == 'B') { // Xóa ký tự
    if (input.length() > 0) {
      input.remove(input.length()-1);
      lcd.setCursor(input.length(),1);
      lcd.print(" ");
      lcd.setCursor(input.length(),1);
    }
  }
  else if (key == 'C') { // Đổi mật khẩu
    mode = CHANGE_OLD;
    input = "";
    lcd.clear();
    lcd.setCursor(0,0);
    lcd.print("Nhap MK cu:");
  }
  else if (key == 'D') { // Reset mật khẩu
    lcd.clear();
    lcd.setCursor(0,0);
    lcd.print("Reset MK?");
    lcd.setCursor(0,1);
    lcd.print("#=Yes  *=No");

    while (true) {
      char confirmKey = keypad.getKey();
      if (confirmKey) {
        if (confirmKey == '#') { 
          lcd.clear();
          lcd.setCursor(0,0);
          lcd.print("Nhap PIN:");
          String enteredPin = "";

          while (true) {
            char pinKey = keypad.getKey();
            if (pinKey) {
              if (pinKey >= '0' && pinKey <= '9') {
                enteredPin += pinKey;
                lcd.setCursor(enteredPin.length()-1,1);
                lcd.print("*");
              } else if (pinKey == 'A') {
                if (enteredPin == pinReset) {
                  resetPassword();          
                  password = loadPassword(); 
                  Serial.println("Reset thanh cong, password = 1234");
                  lcd.clear();
                  lcd.setCursor(0,0);
                  lcd.print("Reset thanh cong!");
                } else {
                  lcd.clear();
                  lcd.setCursor(0,0);
                  lcd.print("PIN sai!");
                }
                delay(2000);
                resetInput();
                break;
              } else if (pinKey == 'B') {
                if (enteredPin.length() > 0) {
                  enteredPin.remove(enteredPin.length()-1);
                  lcd.setCursor(enteredPin.length(),1);
                  lcd.print(" ");
                  lcd.setCursor(enteredPin.length(),1);
                }
              }
            }
          }
          break;
        } else if (confirmKey == '*') { 
          lcd.clear();
          resetInput();
          break;
        }
      }
    }
  }
  else { // nhập ký tự thường
    if (input.length() < 16) {
      input += key;
      lcd.setCursor(input.length()-1,1);
      lcd.print("*");
    }
  }
}

void autoCloseIfNoPerson() {
  if (waitForPerson && !personDetected) {
    unsigned long elapsed = millis() - openTime;
    int remaining = countdownTime - (elapsed / 1000);

    if (remaining >= 0) {
      // Hiển thị đếm ngược
      lcd.setCursor(0,0);
      lcd.print("Khong co nguoi ");
      lcd.setCursor(0,1);
      lcd.print("Dong sau: ");
      lcd.print(remaining);
      lcd.print("s  ");
    }

    if (elapsed >= autoCloseTime) {
      Serial.println("Khong co nguoi -> Tu dong dong cua sau 10s");
      lockServo.write(70); // đóng cửa
      waitForPerson = false;
      personDetected = false;
      lcd.clear();lcd.setCursor(0,0);lcd.print("Cua dong lai");
      delay(1500);
      resetInput();
    }
  }
}
void beepError() {
  for (int i = 0; i < 3; i++) {   
    tone(BUZZER_PIN, 2000);   
    delay(500);    
    noTone(BUZZER_PIN);      
    delay(500);
  }
}
// --- Hàm đọc MQ2 ---
float getMQ2Rs(){
  int adc = analogRead(MQ2_PIN);
  float vout = adc*(VCC/ADC_RES);
  return RL*(VCC-vout)/vout;
}
float getCOPPM(float rs_ro_ratio) {
  // Dựa trên datasheet MQ-2
  float x1 = log10(100);   float y1 = log10(3.0);
  float x2 = log10(1000);  float y2 = log10(0.4);
  float a = (y2 - y1) / (x2 - x1);
  float b = y1 - a * x1;
  float ppm = pow(10, (log10(rs_ro_ratio) - b) / a);
  return ppm;
}
void sendSensor() {
  // Đọc DHT22
  float t = dht.readTemperature();
  float h = dht.readHumidity();
  // MQ-2
  float rs = getMQ2Rs();
  float rs_ro_ratio = rs / R0;
  float co_ppm = getCOPPM(rs_ro_ratio);
  // Hiển thị OLED
  display.clearDisplay();
  display.setCursor(0,0);
  display.println("MOI TRUONG TRONG NHA:");
  display.printf("T:%.1fC\n",t);
  display.printf("H:%.1f%%\n",h);
  display.printf("CO:%.1fppm\n",co_ppm);
  if(co_ppm>35 || t>40 || h>90 ){ 
      display.setCursor(70,44); 
      display.println("ALERT!"); 
  }
  display.display();  

  // Gửi dữ liệu lên Blynk
  if(wifiConnected && Blynk.connected()) {
    Blynk.virtualWrite(V0,t);
    Blynk.virtualWrite(V1,h);
    Blynk.virtualWrite(V2,co_ppm);
}
} // <-- Kết thúc hàm sendSensor

BLYNK_CONNECTED() {
  Blynk.syncAll();   
}

BLYNK_WRITE(V3) {
  int buttonState = param.asInt();  // Đọc trạng thái nút Blynk (0 hoặc 1)

  if (buttonState == 1) {           // Khi nhấn ON
    Serial.println("Blynk button pressed -> Mo khoa");
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Mo cua");
    openLock();                     
    resetInput();

    Blynk.virtualWrite(V3, 0);
  }
}

BLYNK_WRITE(V4) {
  int state = param.asInt();  // 1 = ON, 0 = OFF

  if (state == 1) {
    digitalWrite(LED_PIN, HIGH);
    Serial.println("Blynk V4 -> LED ON");
  } else {
    digitalWrite(LED_PIN, LOW);
    Serial.println("Blynk V4 -> LED OFF");
  }
}