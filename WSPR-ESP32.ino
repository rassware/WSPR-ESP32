/*
   Time_NTP.pde
   Example showing time sync to NTP time source

   This sketch uses the Ethernet library
*/


#include <JTEncode.h>
#include <int.h>
#include <TimeLib.h>
#include <WiFi.h>
#include <WiFiUdp.h>
#include <credentials.h>

#include <si5351.h>

Si5351 si5351;

#define TONE_SPACING            146               // ~1.46 Hz
#define WSPR_DELAY              683               // Delay value for WSPR
#define WSPR_CTC                10672             // CTC value for WSPR
#define SYMBOL_COUNT            WSPR_SYMBOL_COUNT
#define CORRECTION              0                 // Freq Correction in Hz
#define TX_LED_PIN              2                 // integrated onboard led




hw_timer_t * timer = NULL;

// WiFi network name and password:
const char * networkName = WIFI_SSID;
const char * networkPswd = WIFI_PASSWD;


JTEncode jtencode;
unsigned long freq =  28124900UL;
char call[6] = "DL2RN";
char loc[5] = "JN68";
uint8_t dbm = 10;
uint8_t tx_buffer[SYMBOL_COUNT];

// NTP Servers:
IPAddress timeServer(185, 255, 121, 15);

WiFiUDP udp;

bool warmup=0;

const int timeZone = 0;     // UTC

unsigned int localPort = 8888;  // local port to listen for UDP packets

void setup()
{

  Serial.begin(9600);

  connectToWiFi(networkName, networkPswd);

  Serial.println("waiting for sync");
  delay(10000);
  setSyncProvider(getNtpTime);
  setSyncInterval(600);
  delay(5000);


  si5351_init();
  Serial.println("OK!");
  delay(5000);

  Serial.print("got time: ");
  printTime();
}

void printTime()
{
  char buf[2];
  sprintf(buf, "%02d", hour());
  Serial.print(buf);
  Serial.print(":");
  sprintf(buf, "%02d", minute());
  Serial.print(buf);
  Serial.print(":");
  sprintf(buf, "%02d", second());
  Serial.println(buf);
}


void loop()
{
  // Trigger every 4 minute
  // WSPR should start on the 1st second of the even minute.
  
  // 30 seconds before trigger enable si5351a output to eliminate startup drift
  if((minute() + 1) % 4 == 0 && second() == 30 && !warmup)
  { 
    warmup=1; //warm up started, bypass this if for the next 30 seconds
    Serial.println("Radio module Warm up Started ...");
    si5351.set_freq(freq * 100ULL, SI5351_CLK0);
    si5351.set_clock_pwr(SI5351_CLK0, 1);  
  }

  if(minute() % 4 == 0 && second() == 0)
  { 
    //time to start encoding
    Serial.print("Start of Transmission Time: ");
    printTime();
    Serial.println("Frequency: " + String(freq * 100ULL));
    encode();
    warmup=0; //reset variable for next warmup cycle wich will start in 4 minutes and 30 seconds
    delay(4000);
  }
}



/*-------- NTP code ----------*/

const int NTP_PACKET_SIZE = 48; // NTP time is in the first 48 bytes of message
byte packetBuffer[NTP_PACKET_SIZE]; //buffer to hold incoming & outgoing packets

time_t getNtpTime()
{
  while (udp.parsePacket() > 0) ; // discard any previously received packets
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
      secsSince1900 =  (unsigned long)packetBuffer[40] << 24;
      secsSince1900 |= (unsigned long)packetBuffer[41] << 16;
      secsSince1900 |= (unsigned long)packetBuffer[42] << 8;
      secsSince1900 |= (unsigned long)packetBuffer[43];
      return secsSince1900 - 2208988800UL + timeZone * SECS_PER_HOUR;
    }
  }
  Serial.println("No NTP Response :-(");
  return 0; // return 0 if unable to get the time
}

// send an NTP request to the time server at the given address
void sendNTPpacket(IPAddress &address)
{
  // set all bytes in the buffer to 0
  memset(packetBuffer, 0, NTP_PACKET_SIZE);
  // Initialize values needed to form NTP request
  // (see URL above for details on the packets)
  packetBuffer[0] = 0b11100011;   // LI, Version, Mode
  packetBuffer[1] = 0;     // Stratum, or type of clock
  packetBuffer[2] = 6;     // Polling Interval
  packetBuffer[3] = 0xEC;  // Peer Clock Precision
  // 8 bytes of zero for Root Delay & Root Dispersion
  packetBuffer[12]  = 49;
  packetBuffer[13]  = 0x4E;
  packetBuffer[14]  = 49;
  packetBuffer[15]  = 52;
  // all NTP fields have been given values, now
  // you can send a packet requesting a timestamp:
  udp.beginPacket(address, 123); //NTP requests are to port 123
  udp.write(packetBuffer, NTP_PACKET_SIZE);
  udp.endPacket();
}

void connectToWiFi(const char * ssid, const char * pwd) {
  Serial.println("Connecting to WiFi network: " + String(ssid));

  // delete old config
  WiFi.disconnect(true);
  //register event handler
  WiFi.onEvent(WiFiEvent);

  //Initiate connection
  WiFi.begin(ssid, pwd);

  Serial.println("Waiting for WIFI connection...");
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
      Serial.println("WiFi lost connection");
      break;
  }
}

void encode()
{
  uint8_t i;
  Serial.println("Sending Beacon");
  digitalWrite(TX_LED_PIN, HIGH);
  jtencode.wspr_encode(call, loc, dbm, tx_buffer);

  // Now do the rest of the message
  for (i = 0; i < SYMBOL_COUNT; i++)
  {
    si5351.set_freq((freq * 100ULL) + (tx_buffer[i] * TONE_SPACING), SI5351_CLK0);
    delay(WSPR_DELAY);
  }

  // Turn off the output
  si5351.set_clock_pwr(SI5351_CLK0, 0);  
  digitalWrite(TX_LED_PIN, LOW);
  Serial.println("Beacon sent");

}

void si5351_init()
{
  // Use the LED as a keying indicator.
  pinMode(TX_LED_PIN, OUTPUT);
  digitalWrite(TX_LED_PIN, HIGH);

  // Initialize the Si5351
  // Change the 2nd parameter in init if using a ref osc other
  // than 25 MHz
  Serial.println("Setup radio module ...");

  si5351.reset();
  // The crystal load value needs to match in order to have an accurate calibration
  si5351.init(SI5351_CRYSTAL_LOAD_8PF, 24000000 , 0);

  // Start on target frequency
  si5351.set_correction(CORRECTION, SI5351_PLL_INPUT_XO);
  
  // Set CLK0 output
  si5351.set_freq(freq * 100ULL, SI5351_CLK0);
  si5351.drive_strength(SI5351_CLK0, SI5351_DRIVE_8MA); // Set for max power
  si5351.set_clock_pwr(SI5351_CLK0, 0); // Disable the clock initially

  Serial.println("Setup successful ...");
  digitalWrite(TX_LED_PIN, LOW);
}
