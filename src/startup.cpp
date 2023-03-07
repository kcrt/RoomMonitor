#include <Arduino.h>
#include <ArduinoOTA.h>
#include <ESPmDNS.h>
#include <ESPping.h>
#include <LittleFS.h>
#include <M5Core2.h>
#include <SD.h>
#include <SensirionI2CScd4x.h>
#include <WiFi.h>

SensirionI2CScd4x scd4x;

#include "myconfig.h"
#include "sound.h"
#include "startup.h"
#include "utils.h"

void draw_screen_info(const char* text, const bool error /*= false*/) {
  M5.Lcd.setTextColor(error ? RED : WHITE);
  M5.Lcd.fillRect(0, 125, 320, 30, BLACK);
  M5.Lcd.drawCentreString(text, 160, 130, 4);
}

void draw_screen_icon(ICON_MODE wifi, ICON_MODE network, ICON_MODE sensor,
                      ICON_MODE mic, ICON_MODE speaker) {
  char icons[5][16] = {"wifi", "network", "sensor", "mic", "speaker"};
  ICON_MODE icon_modes[5] = {wifi, network, sensor, mic, speaker};

  char mode_str[16];
  for (int i = 0; i < 5; i++) {
    switch (icon_modes[i]) {
      case ICON_GRAY:
        strcpy(mode_str, "gray");
        break;
      case ICON_WHITE:
        strcpy(mode_str, "white");
        break;
      case ICON_GREEN:
        strcpy(mode_str, "green");
        break;
      case ICON_RED:
        strcpy(mode_str, "red");
        break;
    }
    char icon_path[32];
    sprintf(icon_path, "/icon_%s_%s.png", icons[i], mode_str);
    M5.Lcd.drawPngFile(LittleFS, icon_path, 26 + (32 + 26) * i, 180);
  }
}

void start_up_screen() {
  M5.Lcd.begin();
  M5.Lcd.setRotation(1);
  M5.Lcd.fillScreen(BLACK);
  M5.Lcd.setTextColor(WHITE);
  M5.Lcd.setTextSize(4);

  if (!LittleFS.begin(false)) {
    M5.Lcd.fillScreen(RED);
    Serial.println("LittleFS not found");
    M5.Lcd.println("LittleFS not found");
    while (true)
      ;
  } else if (!LittleFS.exists("/startup_logo.png")) {
    M5.Lcd.fillScreen(YELLOW);
    Serial.println("Font not found");
    M5.Lcd.println("Font not found");
    while (true)
      ;
  }

  if (!FAST_MODE) M5.Lcd.drawPngFile(LittleFS, "/startup_logo.png", 0, 0);
  M5.Lcd.setTextSize(1);
  draw_screen_info("Starting...");
  draw_screen_icon(ICON_GRAY, ICON_GRAY, ICON_GRAY, ICON_GRAY, ICON_GRAY);
}

void start_wifi() {
  draw_screen_info("Activating WiFi...");
  draw_screen_icon(ICON_WHITE, ICON_GRAY, ICON_GRAY, ICON_GRAY, ICON_GRAY);
  WiFi.mode(WIFI_AP);
  // load WiFi credentials from SD card
  File file = LittleFS.open("/wifi.txt");
  if (!file) {
    draw_screen_icon(ICON_RED, ICON_GRAY, ICON_GRAY, ICON_GRAY, ICON_GRAY);
    halt("no wifi config");
  }
  char wifi_ssid[64] = {0};
  char wifi_password[64] = {0};
  file.readBytesUntil('\n', wifi_ssid, 64);
  file.readBytesUntil('\n', wifi_password, 64);
  file.close();
  WiFi.begin(wifi_ssid, wifi_password);
  int wifi_timeout = 30 * 2;
  draw_screen_info("Connecting Wi-Fi...");
  while (WiFi.status() != WL_CONNECTED && wifi_timeout > 0) {
    delay(500);
    wifi_timeout--;
    draw_screen_icon(wifi_timeout % 2 ? ICON_WHITE : ICON_GRAY, ICON_GRAY,
                     ICON_GRAY, ICON_GRAY, ICON_GRAY);
  }
  if (WiFi.status() != WL_CONNECTED) {
    draw_screen_info("Network timed out", true);
    draw_screen_icon(ICON_RED, ICON_GRAY, ICON_GRAY, ICON_GRAY, ICON_GRAY);
    halt("");
  }
  draw_screen_info("WiFi OK");
  draw_screen_icon(ICON_GREEN, ICON_GRAY, ICON_GRAY, ICON_GRAY, ICON_GRAY);
  Serial.println(WiFi.localIP());
  Serial.println(WiFi.localIPv6());
  sleep(WAIT_FOR_CHECK);
}

void connect_ntp() {
  /* Set RTC from NTP */
  struct tm timeinfo;
  RTC_TimeTypeDef rtc_time;
  RTC_DateTypeDef rtc_date;
  char current_time[32] = {0};
  configTzTime("JST-9", "ntp.nict.jp", "ntp.jst.mfeed.ad.jp");
  if (!getLocalTime(&timeinfo)) {
    draw_screen_info("Cannot get time from NTP", true);
    draw_screen_icon(ICON_GREEN, ICON_RED, ICON_GRAY, ICON_GRAY, ICON_GRAY);
    halt("");
  }

  rtc_date.Year = timeinfo.tm_year + 1900;
  rtc_date.Month = timeinfo.tm_mon + 1;
  rtc_date.Date = timeinfo.tm_mday;
  rtc_time.Hours = timeinfo.tm_hour;
  rtc_time.Minutes = timeinfo.tm_min;
  rtc_time.Seconds = timeinfo.tm_sec;
  M5.Rtc.SetDate(&rtc_date);
  M5.Rtc.SetTime(&rtc_time);
  sprintf(current_time, "RTC: %04d-%02d-%02d %02d:%02d:%02d (JST)",
          rtc_date.Year, rtc_date.Month, rtc_date.Date, rtc_time.Hours,
          rtc_time.Minutes, rtc_time.Seconds);

  draw_screen_info(current_time);
  sleep(WAIT_FOR_CHECK);
}

void connect_network() {
  draw_screen_info("Checking network...");
  draw_screen_icon(ICON_GREEN, ICON_WHITE, ICON_GRAY, ICON_GRAY, ICON_GRAY);

  /* ping to check network */
  if (!FAST_MODE) {
    IPAddress kcrtIp(219, 94, 243, 203);
    if (!ping_start(WiFi.gatewayIP(), 4, 0, 0, 10)) {
      draw_screen_info("Cannot connect to gateway", true);
      draw_screen_icon(ICON_GREEN, ICON_RED, ICON_GRAY, ICON_GRAY, ICON_GRAY);
      halt("");
    } else if (!ping_start(kcrtIp, 4, 0, 0, 10)) {
      draw_screen_info("Cannot connect to kcrt.net", true);
      draw_screen_icon(ICON_GREEN, ICON_RED, ICON_GRAY, ICON_GRAY, ICON_GRAY);
      halt("");
    }
  }

  /* multicast DNS */
  draw_screen_info("multicast DNS...");
  MDNS.begin("roommonitor");

  /* OTA */
  draw_screen_info("Starting OTA...");
  ArduinoOTA.setHostname("roommonitor")
      .onStart([]() {
        String type;
        if (ArduinoOTA.getCommand() == U_FLASH)
          type = "sketch";
        else  // LittleFS
          type = "filesystem";

        LittleFS.end();
        Serial.println("Start updating " + type);
        M5.Lcd.fillScreen(YELLOW);
      })
      .onEnd([]() {
        // restart esp32
        Serial.println("End");
        M5.Lcd.fillScreen(GREEN);
        delay(1000);
        ESP.restart();
      })
      .onProgress([](unsigned int progress, unsigned int total) {
        M5.Lcd.progressBar(0, 0, 320, 240, progress / (total / 100.));
      })
      .onError([](ota_error_t error) {
        M5.Lcd.fillScreen(RED);
        String s;
        switch (error) {
          case OTA_AUTH_ERROR:
            s = "Auth Failed";
            break;
          case OTA_BEGIN_ERROR:
            s = "Begin Failed";
            break;
          case OTA_CONNECT_ERROR:
            s = "Connect Failed";
            break;
          case OTA_RECEIVE_ERROR:
            s = "Receive Failed";
            break;
          case OTA_END_ERROR:
            s = "End Failed";
            break;
          default:
            s = "Unknown Error";
            break;
        }
        Serial.println(s.c_str());
        halt(s.c_str());
      });

  ArduinoOTA.begin();

  Serial.println("Ready");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  /* NTP */
  connect_ntp();

  draw_screen_info("Network OK");
  draw_screen_icon(ICON_GREEN, ICON_GREEN, ICON_GRAY, ICON_GRAY, ICON_GRAY);
  sleep(WAIT_FOR_CHECK);
}

bool measure_scd4x(uint16_t& co2, float& temperature, float& humidity) {
  uint16_t error;
  uint16_t is_data_ready;

  error = scd4x.getDataReadyStatus(is_data_ready);
  if (error || !is_data_ready) return false;

  error = scd4x.readMeasurement(*&co2, *&temperature, *&humidity);
  return error == 0;
}

void start_sensor() {
  /* Check SCD40 Sensor */
  draw_screen_info("Checking sensor...");
  draw_screen_icon(ICON_GREEN, ICON_GREEN, ICON_WHITE, ICON_GRAY, ICON_GRAY);
  scd4x.begin(Wire);
  scd4x.stopPeriodicMeasurement();
  scd4x.startPeriodicMeasurement();
  if (FAST_MODE) {
    // may be incorrect value
    delay(1000);
    scd4x.measureSingleShot();
  } else {
    // wait for sensor to warm up
    for (int i = 0; i < 12; i++) {
      draw_screen_icon(ICON_GREEN, ICON_GREEN, i % 2 ? ICON_GRAY : ICON_WHITE,
                       ICON_GRAY, ICON_GRAY);
      delay(500);
    }
  }
  uint16_t co2;
  float temperature;
  float humidity;
  if (!measure_scd4x(co2, temperature, humidity)) {
    draw_screen_info("Cannot connect to sensor", true);
    draw_screen_icon(ICON_GREEN, ICON_GREEN, ICON_RED, ICON_GRAY, ICON_GRAY);
    halt("");
  }
  char sensor_info[32] = {0};
  sprintf(sensor_info, "Sensor OK [Temp: %.1f C]", temperature);
  draw_screen_info(sensor_info);
  draw_screen_icon(ICON_GREEN, ICON_GREEN, ICON_GREEN, ICON_GRAY, ICON_GRAY);
  sleep(WAIT_FOR_CHECK);
}

void start_mic_speaker() {
  if (FAST_MODE) return;

  const size_t bufsize = 128 * 1024;
  char* buffer = (char*)malloc(bufsize);
  if (!buffer) {
    draw_screen_info("Cannot allocate memory", true);
    draw_screen_icon(ICON_GREEN, ICON_GREEN, ICON_GREEN, ICON_RED, ICON_GRAY);
    halt("");
  }
  draw_screen_info("Checking mic...");
  draw_screen_icon(ICON_GREEN, ICON_GREEN, ICON_GREEN, ICON_WHITE, ICON_GRAY);
  if (!record_audio(buffer, bufsize)) {
    draw_screen_info("Mic error", true);
    draw_screen_icon(ICON_GREEN, ICON_GREEN, ICON_GREEN, ICON_RED, ICON_GRAY);
    halt("");
  }
  draw_screen_info("Mic OK");
  draw_screen_icon(ICON_GREEN, ICON_GREEN, ICON_GREEN, ICON_GREEN, ICON_GRAY);
  sleep(1);

  draw_screen_info("Checking speaker...");
  draw_screen_icon(ICON_GREEN, ICON_GREEN, ICON_GREEN, ICON_GREEN, ICON_WHITE);

  amplify((int16_t*)buffer, bufsize);
  if (!play_audio(buffer, bufsize)) {
    draw_screen_info("Speaker error", true);
    draw_screen_icon(ICON_GREEN, ICON_GREEN, ICON_GREEN, ICON_GREEN, ICON_RED);
    halt("");
  }
  draw_screen_info("Speaker OK");
  draw_screen_icon(ICON_GREEN, ICON_GREEN, ICON_GREEN, ICON_GREEN, ICON_GREEN);
  sleep(1);
  free(buffer);
}