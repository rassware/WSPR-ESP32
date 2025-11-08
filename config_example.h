#define WIFI_SSID "Insert your SSID"
#define WIFI_PASSWD "Insert your password"
#define CALL_SIGN "Insert your callsign"
#define LOCATOR "Insert your locator"
#define CORRECTION 158000  // Freq Correction in Hz
#define TX_LED_PIN 2  // integrated onboard led
#define MAX_MESSAGES 10 // max messages in buffer
#define GMT_OFFSET_SEC 3600 // GMT offset in sec
#define DAYLIGHT_OFFSET_SEC 3600 // daylight offset in sec
#define TIME_SERVER "pool.ntp.org" // URL time server
#define FREQUENCY 2812600000ULL // wspr on 10m is between 2812600000 and 2812620000, we are working between 2812605000 and 2812615000
#define POWER 10 // hf power, default is 10 = 10dBm = 0.01W
#define TRIGGER_EVERY_X_MINUTES 20 // how often should the beacon be sent, valid values between 2 and 60, can be modified via web interface