void start_up_screen();

enum ICON_MODE {
  ICON_GRAY,
  ICON_WHITE,
  ICON_GREEN,
  ICON_RED,
};
void draw_screen_info(const char* text, const bool error = false);
void draw_screen_icon(ICON_MODE wifi, ICON_MODE network, ICON_MODE sensor,
                      ICON_MODE mic, ICON_MODE speaker);
void start_wifi();
void connect_ntp();
void connect_network();
bool measure_scd4x(uint16_t& co2, float& temperature, float& humidity);
void start_sensor();
void start_mic_speaker();