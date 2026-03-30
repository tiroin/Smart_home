#ifndef STORE_FLASH_H
#define STORE_FLASH_H

#include <Arduino.h>
#include <EEPROM.h>

#define ADDR_PASSWORD 0         
#define MAX_PASSWORD_LEN 16
#define DEFAULT_PASSWORD "1234"
#define EEPROM_SIZE 64         

// Khởi tạo EEPROM
void initStorage() {
  if (!EEPROM.begin(EEPROM_SIZE)) {
    Serial.println("EEPROM init failed!");
  } else {
    Serial.println("EEPROM init OK");
  }
}

// Lưu mật khẩu vào EEPROM
void savePassword(String pass) {
  int len = pass.length();
  if (len > MAX_PASSWORD_LEN) len = MAX_PASSWORD_LEN;

  EEPROM.write(ADDR_PASSWORD, len);
  for (int i = 0; i < len; i++) {
    EEPROM.write(ADDR_PASSWORD + 1 + i, pass[i]);
  }
  EEPROM.commit();
}

String loadPassword() {
  int len = EEPROM.read(ADDR_PASSWORD);
  if (len <= 0 || len > MAX_PASSWORD_LEN) return DEFAULT_PASSWORD;

  String pass = "";
  for (int i = 0; i < len; i++) {
    char c = EEPROM.read(ADDR_PASSWORD + 1 + i);
    pass += c;
  }
  return pass;
}

void resetPassword() {
  savePassword(DEFAULT_PASSWORD);
}

bool isValidPassword(String pass) {
  return pass.length() > 0 && pass.length() <= MAX_PASSWORD_LEN;
}

#endif
