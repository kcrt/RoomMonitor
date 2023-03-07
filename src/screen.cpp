
#include <Arduino.h>
#include <M5Core2.h>
#include <SD.h>

#include "myconfig.h"
#include "startup.h"

void init_main_screen() {
  M5.Lcd.fillScreen(BLACK);
  M5.Lcd.setTextColor(TFT_WHITE, TFT_BLACK);
  M5.Lcd.setTextSize(2);
  M5.Lcd.setCursor(0, 0);
  M5.Lcd.println("M5Stack");
  M5.Lcd.println("Audio Test");
  M5.Lcd.println();
  M5.Lcd.println("Press A to start");
  M5.Lcd.println("Press B to exit");
}