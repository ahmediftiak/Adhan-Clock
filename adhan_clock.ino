#include <Arduino.h>
#include <RTClib.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include "LedController.hpp"
#include <AudioFileSourceSD.h>
#include <AudioGeneratorMP3.h>
#include <AudioOutputI2S.h>

// WiFi credentials
const char* ssid = "";
const char* password = "";

// I2S Connections
#define I2S_BCLK 1
#define I2S_DOUT 2
#define I2S_LRC 3

const int chipSelect = 21;         // SD card's chip select pin
const char* fileName = "/a1.mp3";  // Audio file path

// Audio objects
AudioGeneratorMP3* mp3;
AudioFileSourceSD* file;
AudioOutputI2S* out;

// RTC and LED matrix objects
RTC_DS3231 rtc;
LedController<1, 1> lc(7, 9, 8);

// Adhan timings and related variables
const int maxAdhanTimings = 5;
int adhanTimings[maxAdhanTimings];
int testtimings[] = {1355, 1360, 1365};

String lastFetchDate = "";
bool blinkTime = false;
bool isLEDOn = true;
bool playing = false;
unsigned long lastBlinked = 0;
DateTime now;

// Function prototypes
void printTime(DateTime now);
void setAudio();
void connectWiFi();
void fetchTimings(String date);
int timeToMinutes(const char* timeStr);
void checkAdhanTimings();
void testAdhan(DateTime now);
void displayCurrentTime(DateTime now);
void blinkDisplay();

void setup() {
  Serial.begin(115200);       // Initialize serial communication
  rtc.begin();                // Initialize RTC
  lc.setIntensity(8);         // Set LED intensity
  lc.clearMatrix();           // Clear LED Matrix

  connectWiFi();              // Connect to WiFi
  SD.begin(chipSelect);       // Initialize SD card

  // Initialize audio output and generator
  out = new AudioOutputI2S();
  out->SetPinout(I2S_BCLK, I2S_LRC, I2S_DOUT);
  mp3 = new AudioGeneratorMP3();
}

void loop() {
  now = rtc.now();  // Get the current time
  String newDate = String(now.day(), 2) + "-" + String(now.month(), 2) + "-" + String(now.year(), 4); // Format date as DD-MM-YYYY

  if (newDate != lastFetchDate) {  // Fetch new timings if the date has changed
    fetchTimings(newDate);
    lastFetchDate = newDate;
  }
  
  checkAdhanTimings();  // Check if it's time for Adhan
  if (!blinkTime) {
    displayCurrentTime(now);  // Display current time on LED Matrix if not blinking
  }
}

void printTime(DateTime now) {
  // Print the current time to the serial monitor
  Serial.printf("Time: %02d:%02d:%02d\n", now.twelveHour(), now.minute(), now.second());
  delay(1000);
}

void connectWiFi() {
  // Connect to WiFi network
  WiFi.begin(ssid, password);
  Serial.println("Connecting to WiFi...");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.printf("\nConnected to WiFi network with IP Address: %s\n", WiFi.localIP().toString().c_str());
}

void fetchTimings(String date) {
  // Fetch prayer timings from the API based on the given date
  if (WiFi.status() != WL_CONNECTED) {
    connectWiFi();  // Reconnect WiFi if not connected
  }
  
  HTTPClient http;
  String url = "http://api.aladhan.com/v1/timingsByCity/" + date + "?city=new+york&country=United+State&method=2";
  
  if (http.begin(url)) {
    int httpCode = http.GET();
    if (httpCode == HTTP_CODE_OK) {
      // Parse JSON response
      String payload = http.getString();
      DynamicJsonDocument doc(2048);
      deserializeJson(doc, payload);
      JsonObject data_timings = doc["data"]["timings"];
      
      // Store adhan timings in the array
      adhanTimings[0] = timeToMinutes(data_timings["Fajr"]);
      adhanTimings[1] = timeToMinutes(data_timings["Dhuhr"]);
      adhanTimings[2] = timeToMinutes(data_timings["Asr"]);
      adhanTimings[3] = timeToMinutes(data_timings["Maghrib"]);
      adhanTimings[4] = timeToMinutes(data_timings["Isha"]);
      
      // Print adhan timings to Serial Monitor
      Serial.println("Adhan Timings (in minutes):");
      for (int i = 0; i < maxAdhanTimings; i++) {
        Serial.println(adhanTimings[i]);
      }
      
      // Print adhan timings as formatted time
      Serial.println("Adhan Timings (formatted):");
      for (int i = 0; i < maxAdhanTimings; i++) {
        Serial.printf("%02d:%02d\n", adhanTimings[i] / 60, adhanTimings[i] % 60);
      }
    } else {
      Serial.printf("[HTTP] GET failed, error: %s\n", http.errorToString(httpCode).c_str());
    }
    http.end();
  } else {
    Serial.println("Unable to connect to API");
  }
}

int timeToMinutes(const char* timeStr) {
  // Convert a time string to minutes
  int hours, minutes;
  sscanf(timeStr, "%d:%d", &hours, &minutes);
  return hours * 60 + minutes;
}

void playAdhan() {
  // Play the Adhan audio
  if (mp3->isRunning()) {
    if (!mp3->loop()) {
      mp3->stop();
      Serial.println("Stopped");
      playing = false;
    }
  }
}

void loadfile() {
  // Load the Adhan audio file
  file = new AudioFileSourceSD(fileName);
  out->SetGain(0.08);
  mp3->begin(file, out);
  playing = true;
}

void checkAdhanTimings() {
  // Check if it's time for a prayer call (Adhan)
  int currentMinutes = now.hour() * 60 + now.minute();
  for (int i = 0; i < maxAdhanTimings; i++) {
    if (currentMinutes == adhanTimings[i] && now.second() < 6) {
      blinkTime = true;
      blinkDisplay();
    } else {
      blinkTime = false;
    }
    if (currentMinutes == adhanTimings[i] || playing) {
      if (!playing) loadfile();
      playAdhan();
      break;
    }
  }
}

void testAdhan(DateTime now) {
  // Test if it's time for a simulated prayer call
  int currentMinutes = now.hour() * 60 + now.minute();
  for (int i = 0; i < sizeof(testtimings) / sizeof(testtimings[0]); i++) {
    if (currentMinutes == testtimings[i] && now.second() < 6) {
      blinkTime = true;
      blinkDisplay();
    } else {
      blinkTime = false;
    }
    if (currentMinutes == testtimings[i] || playing) {
      if (!playing) loadfile();
      playAdhan();
      break;
    }
  }
}

void displayCurrentTime(DateTime now) {
  // Display the current time on LED Matrix
  int hour = now.twelveHour();
  int minute = now.minute();
  if (hour < 10) {
    lc.setDigit(0, 1, hour, false);  // Display the ones digit of the hour
  } else {
    lc.setDigit(0, 0, hour / 10, false);  // Display the tens digit of the hour
    lc.setDigit(0, 1, hour % 10, false);  // Display the ones digit of the hour
  }
  lc.setDigit(0, 2, minute / 10, false);  // Display the tens digit of the minute
  lc.setDigit(0, 3, minute % 10, false);  // Display the ones digit of the minute
}

void blinkDisplay() {
  // Blink the LED Matrix display
  unsigned long currentMillis = millis();
  if (currentMillis - lastBlinked >= 500) {
    lastBlinked = currentMillis;
    if (isLEDOn) {
      lc.clearMatrix();  // Clear LED Matrix if it's on
      isLEDOn = false;
    } else {
      displayCurrentTime(now);  // Display current time if LED is off
      isLEDOn = true;
    }
  }
}
