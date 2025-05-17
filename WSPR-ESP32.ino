/*
  WSPR-ESP32.ino
  created 2025-04-27
  by Christian Rassmann


  Uses
  Etherkit JTEncode by Jason Mildrum
  Etherkit Si5351 by Jason Mildrum
  Time by Michael Margolis


*/


#include <JTEncode.h>
#include <int.h>
#include <TimeLib.h>
#include <WiFi.h>
#include <WiFiUdp.h>
#include <config.h>
#include <si5351.h>
#include <ArduinoOTA.h>


#define TONE_SPACING 146  // ~1.46 Hz
#define WSPR_DELAY 683    // Delay value for WSPR
#define WSPR_CTC 10672    // CTC value for WSPR
#define SYMBOL_COUNT WSPR_SYMBOL_COUNT

Si5351 si5351;
JTEncode jtencode;
IPAddress timeServer(185, 255, 121, 15);
WiFiUDP udp;
String messageBuffer[MAX_MESSAGES];

// WiFi network name and password:
const char *networkName = WIFI_SSID;
const char *networkPswd = WIFI_PASSWD;

hw_timer_t *timer = NULL;
unsigned long freq = 2812610000ULL;
char call[6] = CALL_SIGN;
char loc[5] = LOCATOR;
uint8_t dbm = 10;
uint8_t tx_buffer[SYMBOL_COUNT];
bool warmup = 0;
bool active = true;
uint8_t localPort = 8888;  // local port to listen for UDP packets
uint8_t trigger_every_x_minutes = 20; // how often should the beacon be sent
int messageStart = 0;   // index of the oldest message
int messageCount = 0;   // count of messages


void setup() {
  Serial.begin(9600);
  connectToWiFi(networkName, networkPswd);
  log("waiting for sync");
  delay(10000);
  setSyncProvider(getNtpTime);
  setSyncInterval(3600);
  delay(5000);
  si5351_init();
  delay(5000);
  log("got time: " + String(printTime()));
  webserver_setup();
  // OTA Setup
  ArduinoOTA
    .onStart([]() {
      String type;
      if (ArduinoOTA.getCommand() == U_FLASH)
        type = "Sketch";
      else // U_SPIFFS
        type = "Filesystem";
      Serial.println("Start OTA Update: " + type);
    })
    .onEnd([]() {
      Serial.println("\nOTA Update finished.");
    })
    .onProgress([](unsigned int progress, unsigned int total) {
      Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
    })
    .onError([](ota_error_t error) {
      Serial.printf("Error [%u]: ", error);
      if (error == OTA_AUTH_ERROR) Serial.println("auth error");
      else if (error == OTA_BEGIN_ERROR) Serial.println("begin error");
      else if (error == OTA_CONNECT_ERROR) Serial.println("connect error");
      else if (error == OTA_RECEIVE_ERROR) Serial.println("receive error");
      else if (error == OTA_END_ERROR) Serial.println("end error");
    });
  ArduinoOTA.setHostname("WSPR-ESP32");
  ArduinoOTA.setPassword("wspr");
  ArduinoOTA.begin();
}

char *printTime() {
  static char buf[9];
  sprintf(buf, "%02d:%02d:%02d\0", hour(), minute(), second());
  return buf;
}

void loop() {
  // Trigger every X minutes defined by variable 'trigger_every_x_minutes'
  // WSPR should start on the 1st second of the even minute.

  // 50 seconds before trigger enable si5351a output to eliminate startup drift
  if ((minute() + 1) % trigger_every_x_minutes == 0 && second() == 50 && !warmup && active) {
    warmup = 1;  //warm up started, bypass this if for the next 10 seconds
    log("Radio module warm up started ...");
    si5351.set_freq(freq, SI5351_CLK0);
    si5351.set_clock_pwr(SI5351_CLK0, 1);
  }

  if (minute() % trigger_every_x_minutes == 0 && second() == 0 && active) {
    //time to start encoding
    log("Start of Transmission Time: " + String(printTime()));
    log("Frequency: " + String(freq));
    encode();
    warmup = 0;  //reset variable for next warmup cycle wich will start in x minutes and 50 seconds
    delay(4000);
  }
   ArduinoOTA.handle();
}



/*-------- NTP code ----------*/

const int NTP_PACKET_SIZE = 48;      // NTP time is in the first 48 bytes of message
byte packetBuffer[NTP_PACKET_SIZE];  //buffer to hold incoming & outgoing packets

time_t getNtpTime() {
  while (udp.parsePacket() > 0)
    ;  // discard any previously received packets
  Serial.println("Transmit NTP Request");
  sendNTPpacket(timeServer);
  uint32_t beginWait = millis();
  while (millis() - beginWait < 1500) {
    int size = udp.parsePacket();
    if (size >= NTP_PACKET_SIZE) {
      Serial.println("Receive NTP Response");
      udp.read(packetBuffer, NTP_PACKET_SIZE);  // read packet into the buffer
      unsigned long secsSince1900;
      // convert four bytes starting at location 40 to a long integer
      secsSince1900 = (unsigned long)packetBuffer[40] << 24;
      secsSince1900 |= (unsigned long)packetBuffer[41] << 16;
      secsSince1900 |= (unsigned long)packetBuffer[42] << 8;
      secsSince1900 |= (unsigned long)packetBuffer[43];
      return secsSince1900 - 2208988800UL;
    }
  }
  Serial.println("No NTP Response :-(");
  return 0;  // return 0 if unable to get the time
}

// send an NTP request to the time server at the given address
void sendNTPpacket(IPAddress &address) {
  // set all bytes in the buffer to 0
  memset(packetBuffer, 0, NTP_PACKET_SIZE);
  // Initialize values needed to form NTP request
  // (see URL above for details on the packets)
  packetBuffer[0] = 0b11100011;  // LI, Version, Mode
  packetBuffer[1] = 0;           // Stratum, or type of clock
  packetBuffer[2] = 6;           // Polling Interval
  packetBuffer[3] = 0xEC;        // Peer Clock Precision
  // 8 bytes of zero for Root Delay & Root Dispersion
  packetBuffer[12] = 49;
  packetBuffer[13] = 0x4E;
  packetBuffer[14] = 49;
  packetBuffer[15] = 52;
  // all NTP fields have been given values, now
  // you can send a packet requesting a timestamp:
  udp.beginPacket(address, 123);  //NTP requests are to port 123
  udp.write(packetBuffer, NTP_PACKET_SIZE);
  udp.endPacket();
}

void connectToWiFi(const char *ssid, const char *pwd) {
  log("Connecting to WiFi network: " + String(ssid));

  // delete old config
  WiFi.disconnect(true);
  //register event handler
  WiFi.onEvent(WiFiEvent);

  //Initiate connection
  WiFi.begin(ssid, pwd);

  log("Waiting for WIFI connection...");
}

//wifi event handler
void WiFiEvent(WiFiEvent_t event) {
  switch (event) {
    case ARDUINO_EVENT_WIFI_STA_GOT_IP:
      //When connected set
      Serial.print("WiFi connected! IP address: ");
      Serial.println(WiFi.localIP());
      //initializes the UDP state
      //This initializes the transfer buffer
      udp.begin(WiFi.localIP(), localPort);
      break;
    case ARDUINO_EVENT_WIFI_STA_DISCONNECTED:
      log("WiFi lost connection");
      break;
  }
}

void encode() {
  uint8_t i;
  log("Sending beacon");
  digitalWrite(TX_LED_PIN, HIGH);
  jtencode.wspr_encode(call, loc, dbm, tx_buffer);

  // Now do the rest of the message
  for (i = 0; i < SYMBOL_COUNT; i++) {
    si5351.set_freq(freq + (tx_buffer[i] * TONE_SPACING), SI5351_CLK0);
    delay(WSPR_DELAY);
  }

  // Turn off the output
  si5351.set_clock_pwr(SI5351_CLK0, 0);
  digitalWrite(TX_LED_PIN, LOW);
  log("Beacon sent");
}

void si5351_init() {
  // Use the LED as a keying indicator.
  pinMode(TX_LED_PIN, OUTPUT);
  digitalWrite(TX_LED_PIN, HIGH);

  // Initialize the Si5351
  // Change the 2nd parameter in init if using a ref osc other
  // than 25 MHz
  Serial.println("Setup radio module ...");

  si5351.reset();
  // The crystal load value needs to match in order to have an accurate calibration
  si5351.init(SI5351_CRYSTAL_LOAD_8PF, 0, 0);

  // Start on target frequency
  si5351.set_correction(CORRECTION, SI5351_PLL_INPUT_XO);

  // Set CLK0 output
  si5351.set_freq(freq, SI5351_CLK0);
  si5351.drive_strength(SI5351_CLK0, SI5351_DRIVE_8MA);  // Set for max power
  si5351.set_clock_pwr(SI5351_CLK0, 0);                  // Disable the clock initially

  Serial.println("Setup successful ...");
  digitalWrite(TX_LED_PIN, LOW);
}

void log(String message) {
  Serial.println(message);
  String json = "{\"timestamp\": \"" + String(printTime()) + "\", \"message\": \"" + message + "\"}";
  addMessageToBuffer(json);
  ws_sendAll(json);
}

char * si5351_get_status() {
  static char buf[50];
  si5351.update_status();
  sprintf(buf, "SYS_INIT: %" PRIu8 ", LOL_A: %" PRIu8 ", LOL_B: %" PRIu8 ", LOS: %" PRIu8 ", REVID: %" PRIu8, si5351.dev_status.SYS_INIT, si5351.dev_status.LOL_A, si5351.dev_status.LOL_B, si5351.dev_status.LOS, si5351.dev_status.REVID);
  return buf;
}

void addMessageToBuffer(String message) {
  int index = (messageStart + messageCount) % MAX_MESSAGES;
  messageBuffer[index] = message;
  if (messageCount < MAX_MESSAGES) {
    messageCount++;
  } else {
    messageStart = (messageStart + 1) % MAX_MESSAGES; // override oldest message
  }
}
