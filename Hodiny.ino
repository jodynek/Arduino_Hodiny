#include <NTPClient.h>
#include <ESP8266WiFi.h>
#include <WiFiUdp.h>
#include <TimeAlarms.h>
#include <TimeLib.h>
#include <TM1637Display.h>
#include <Adafruit_NeoPixel.h>
#define PIN 2
#define NUM_LEDS 24
// Parameter 1 = number of pixels in strip
// Parameter 2 = pin number (most are valid)
// Parameter 3 = pixel type flags, add together as needed:
//   NEO_KHZ800  800 KHz bitstream (most NeoPixel products w/WS2812 LEDs)
//   NEO_KHZ400  400 KHz (classic 'v1' (not v2) FLORA pixels, WS2811 drivers)
//   NEO_GRB     Pixels are wired for GRB bitstream (most NeoPixel products)
//   NEO_RGB     Pixels are wired for RGB bitstream (v1 FLORA pixels, not v2)
Adafruit_NeoPixel strip = Adafruit_NeoPixel(NUM_LEDS, PIN, NEO_GRB + NEO_KHZ800);

// WiFi
WiFiUDP ntpUDP;   // NTP
const char* ssid = "Jodynek";
const char* password = "Mokelembembe77";
NTPClient timeClient(ntpUDP, "europe.pool.ntp.org", 3600, 60000);
WiFiClient client;

// Piny pro pripojeni displeje
#define CLK 5
#define DIO 4

// pripojeni bzucaku
#define BUZZER 0

// Definice specialnich znaku
#define CHAR_LINES 0b01001001
#define CHAR_MINUS 0b01000000
#define CHAR_DEG   0b01100011
#define CHAR_C     0b00111001
#define CHAR_F     0b01110001
#define CHAR_E     0b01111001
#define CHAR_R     0b01010000

// Inicializace displeje
TM1637Display display(CLK, DIO);

bool showDots = true;
bool beeped = false;
bool flashHalfHour = false;

void setup() {
  Serial.begin(115200);

  // strip LED
  strip.begin();
  strip.show(); // Initialize all pixels to 'off'

  // bzucak
  pinMode(BUZZER, OUTPUT);

  // Nastaveni jasu: 0 (min) - 7 (max)
  display.setBrightness(4);

  // Connect to WiFi network
  Serial.println();
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);
  if (WiFi.status() != WL_CONNECTED) {
    WiFi.begin(ssid, password);
  }
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected");

  // nastaveni datumu a casu
  timeClient.begin();
  while (timeStatus() == timeNotSet || year() == 1970)
  {
    timeClient.begin();
    timeClient.update();
    setTime(timeClient.getEpochTime());
    Serial.print(".");
    delay(1000);
  }
  Serial.println("\nDate and time set");

  beep();
  // LED strip - white color
  CylonBounce(0xff, 0xff, 0xff, 4, 70, 50);
  setAll(0,0,0); // Set all black      
}

void loop() {
  // je potreba kvuli synchronizaci alarmu
  Alarm.delay(0);
  // show date&time
  const unsigned long interval = 1000UL;
  // 1 sec.
  static unsigned long lastSampleTime = 0 - interval;
  unsigned long now_time = millis();
  // kazda 1 sec. refresh casu
  if (now_time - lastSampleTime >= interval)
  {
    lastSampleTime += interval;
    
    // aktualni cas
    char buffer[10];
    sprintf(buffer, "%02d%02d", hour() + 1, minute());
    int time = atoi(buffer);
    Serial.println(time);
    // kazdou sekundu blikne dvojtecka mezi hodinou a minutama
    if (showDots)
      display.showNumberDecEx(time, 0b01000000, true);
    else
      display.showNumberDecEx(time, 0, true);

    // kazdou celou hodinu pipneme
    if (minute() == 0 && !beeped) {
      // LED strip - red color
      CylonBounce(0xff, 0, 0, 4, 70, 50);
      setAll(0,0,0); // Set all black
      // Sound
      beep();
      beeped = true;
    } else if (minute() == 30 && !flashHalfHour) {
      // LED strip - white color
      CylonBounce(0xff, 0xff, 0xff, 4, 70, 50);
      setAll(0,0,0); // Set all black      
      flashHalfHour = true;
    }
    
    if (minute() > 0 && beeped) 
      beeped = false;
    if (minute() > 30 && flashHalfHour) 
      flashHalfHour = false;

    showDots = !showDots;
  }

  delay(5);
}


void CylonBounce(byte red, byte green, byte blue, int EyeSize, int SpeedDelay, int ReturnDelay)
{
  for(int i = 0; i < NUM_LEDS-EyeSize-2; i++) {
    setAll(0,0,0);
    setPixel(i, red/10, green/10, blue/10);
    for(int j = 1; j <= EyeSize; j++) {
      setPixel(i+j, red, green, blue);
    }
    setPixel(i+EyeSize+1, red/10, green/10, blue/10);
    showStrip();
    delay(SpeedDelay);
  }

  delay(ReturnDelay);

  for(int i = NUM_LEDS-EyeSize-2; i > 0; i--) {
    setAll(0,0,0);
    setPixel(i, red/10, green/10, blue/10);
    for(int j = 1; j <= EyeSize; j++) {
      setPixel(i+j, red, green, blue);
    }
    setPixel(i+EyeSize+1, red/10, green/10, blue/10);
    showStrip();
    delay(SpeedDelay);
  }
 
  delay(ReturnDelay);
}

void showStrip() 
{
 #ifdef ADAFRUIT_NEOPIXEL_H
   // NeoPixel
   strip.show();
 #endif
 #ifndef ADAFRUIT_NEOPIXEL_H
   // FastLED
   FastLED.show();
 #endif
}

void setPixel(int Pixel, byte red, byte green, byte blue) 
{
 #ifdef ADAFRUIT_NEOPIXEL_H
   // NeoPixel
   strip.setPixelColor(Pixel, strip.Color(red, green, blue));
 #endif
 #ifndef ADAFRUIT_NEOPIXEL_H
   // FastLED
   leds[Pixel].r = red;
   leds[Pixel].g = green;
   leds[Pixel].b = blue;
 #endif
}

void setAll(byte red, byte green, byte blue) 
{
  for(int i = 0; i < NUM_LEDS; i++ ) {
    setPixel(i, red, green, blue);
  }
  showStrip();
}

void beep()
{
  for (int fadeValue = 155 ; fadeValue >= 0; fadeValue -= 5) {
    analogWrite(BUZZER, fadeValue);
    delay(1);
  }
  delay(200);
  for (int fadeValue = 155 ; fadeValue >= 0; fadeValue -= 5) {
    analogWrite(BUZZER, fadeValue);
    delay(1);
  }
  Serial.println("Beep");
}
