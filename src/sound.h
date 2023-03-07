
bool init_i2s(bool mic);
bool record_audio(void* buffer, size_t bufsize);
bool play_audio(const void* buffer, size_t bufsize);
void amplify(int16_t* buffer, size_t bufsize);