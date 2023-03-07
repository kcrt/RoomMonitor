#include <Arduino.h>
#include <ArduinoOTA.h>
#include <M5Core2.h>

#include "myconfig.h"
#include "sound.h"
#include "sound/click.h"
#include "sound/panic.h"
#include "startup.h"

void setup() {
  M5.begin(true /* LCD */, true /* SD */, true /* Serial */, true /* I2C */);

  Serial.begin(115200);
  Serial.println("");
  Serial.println("kcrt's RoomMonitor");

  M5.Rtc.begin();

  start_up_screen();
  start_wifi();
  connect_network();
  start_sensor();
  start_mic_speaker();
  draw_screen_info("System all green!");

  sleep(WAIT_FOR_CHECK);
}

static long count = 0;
static bool panicflag = false;
void loop() {
  ArduinoOTA.handle();

  if (count % 120 == 0) {
    uint16_t co2;
    float temperature, humidity;
    measure_scd4x(co2, temperature, humidity);
    M5.Lcd.clear(BLACK);
    M5.Lcd.setTextColor(WHITE);

    M5.Lcd.drawRightString("CO2[ppm]: ", 150, 30, 4);
    M5.Lcd.drawRightString("Temp[C]: ", 150, 100, 4);
    M5.Lcd.drawRightString("Humid[%]: ", 150, 170, 4);

    M5.Lcd.setTextColor(co2 > 1000 ? RED : co2 > 800 ? YELLOW : WHITE);
    M5.Lcd.drawRightString(String(co2), 300, 10, 7);
    M5.Lcd.setTextColor(WHITE);
    M5.Lcd.drawRightString(String(temperature, 1), 300, 80, 7);
    M5.Lcd.drawRightString(String(humidity, 1), 300, 150, 7);

    char batStr[32];
    sprintf(batStr, "Battery: %.1f %%", M5.Axp.GetBatteryLevel());
    M5.Lcd.drawString(batStr, 0, 220, 1);
    if (!panicflag & co2 > 1200) {
      play_audio(SOUND_PANIC, sizeof(SOUND_PANIC));
    } else {
      play_audio(SOUND_CLICK, sizeof(SOUND_CLICK));
    }
    if (co2 < 1000) panicflag = false;
    count = 0;
  } else {
    const uint8_t a = 255 - 255 * count / 120; /* 255 -> 0 */
    const uint16_t active_color = M5.Lcd.color565(a, a, a);
    M5.Lcd.fillCircle(320 - 10, 240 - 10, 5, count % 2 ? active_color : BLACK);
    M5.Lcd.drawCircle(320 - 10, 240 - 10, 5, WHITE);
    delay(500);
  }
  count++;
}