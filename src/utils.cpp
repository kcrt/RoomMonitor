
#include <M5Core2.h>

void halt(const char* message) {
  M5.Lcd.setTextColor(RED);
  M5.Lcd.setTextSize(1);
  M5.Lcd.drawCentreString(message, 160, 16, 4);
  while (true) {
    sleep(1);
  }
}