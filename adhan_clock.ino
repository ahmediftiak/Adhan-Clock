#include <Arduino.h>
#include <RTClib.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include "LedController.hpp"
#include <AudioFileSourceSD.h>
#include <AudioGeneratorMP3.h>
#include <AudioOutputI2S.h>
#include "time.h"

// Pin Definitions for I2S audio output and LED control
#define I2S_BCLK 3
#define I2S_DOUT 4
#define I2S_LRC 2
#define DIN 5
#define CLK 6
#define CS 7
#define CHIP_SELECT 18

// WiFi and Time Configuration
const char* ssid = "";
const char* password = "";
const char* fileName = "/a1.mp3";        // MP3 file to play during Adhan
const char* ntpServer = "pool.ntp.org";  // NTP server for time synchronization
const long gmtOffset_sec = -18000;       // GMT Offset for EST (Eastern Standard Time)
const int daylightOffset_sec = 3600;     // Daylight saving time offset in seconds
const int maxAdhanTimings = 5;           // Number of Adhan timings (Fajr, Dhuhr, Asr, Maghrib, Isha)

// Audio and RTC setup
AudioGeneratorMP3* mp3;
AudioFileSourceSD* file;
AudioOutputI2S* out;
RTC_DS3231 rtc;                        // RTC object for time management
LedController<1, 1> lc(DIN, CLK, CS);  // LED controller for displaying time

// Global variables for managing Adhan timings, blinking, and MP3 playback
int testTimings[] = {1330, 1345, 1360, 1375};
int adhanTimings[maxAdhanTimings];                                // Array to store Adhan timings in minutes
char lastFetchDate[11] = "";                                      // Store the last fetched date to avoid redundant API calls
bool isBlinking = false, isLEDOn = true, isAdhanPlaying = false;  // Flags for blinking, LED state, and Adhan playback
unsigned long lastBlinkTime = 0;                                  // Time tracking for blinking effect

// Function to connect to WiFi
void connectWiFi() {
  WiFi.begin(ssid, password);  // Start WiFi connection
  Serial.println("Connecting to WiFi...");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);  // Wait for WiFi to connect
    Serial.print(".");
  }
  Serial.printf("\nConnected to WiFi network with IP Address: %s\n", WiFi.localIP().toString().c_str());
}

// Function to synchronize the device's time with an NTP server
void syncTime() {
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);  // Configure time with NTP server
  struct tm timeinfo;
  if (getLocalTime(&timeinfo)) {  // Check if time is available
    Serial.println(&timeinfo, "%A, %B %d %Y %H:%M:%S");
    rtc.adjust(DateTime(timeinfo.tm_year + 1900, timeinfo.tm_mon + 1, timeinfo.tm_mday, timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec));  // Sync RTC
  } else {
    Serial.println("Failed to obtain time");
  }
}

// Function to convert a time string "HH:MM" to minutes
int timeToMinutes(const char* timeStr) {
  int h, m;
  sscanf(timeStr, "%d:%d", &h, &m);  // Parse hours and minutes
  return h * 60 + m;                 // Return total minutes
}

// Function to fetch the prayer timings for the current day
void fetchTimings(String date) {
  if (WiFi.status() != WL_CONNECTED) {
    connectWiFi();  // Reconnect WiFi if not connected
  }
  HTTPClient http;
  String url = "https://api.aladhan.com/v1/timingsByCity/" + date + "?city=New+York+City&country=US&method=2&shafaq=general&school=0&midnightMode=0";

  if (http.begin(url)) {               // Start HTTP GET request
    if (http.GET() == HTTP_CODE_OK) {  // If request is successful
      DynamicJsonDocument doc(2048);
      deserializeJson(doc, http.getString());  // Parse JSON response
      JsonObject timings = doc["data"]["timings"];
      const char* keys[] = { "Fajr", "Dhuhr", "Asr", "Maghrib", "Isha" };
      // Store the timings in the adhanTimings array
      for (int i = 0; i < maxAdhanTimings; i++) adhanTimings[i] = timeToMinutes(timings[keys[i]]);

      // Print the formatted Adhan timings to the Serial Monitor
      Serial.println("Adhan Timings (formatted):");
      for (int i = 0; i < maxAdhanTimings; i++) {
        Serial.printf("%02d:%02d\n", adhanTimings[i] / 60, adhanTimings[i] % 60);
      }
    } else {
      Serial.printf("[HTTP] GET failed, error: %s\n", http.errorToString(http.GET()).c_str());
    }
    http.end();
  } else {
    Serial.println("Unable to connect to API");
  }
}

// Function to load the MP3 file from SD card
void loadfile() {
  file = new AudioFileSourceSD(fileName);  // Create an MP3 file source object
  out->SetGain(0.7);                       // Set audio gain
  mp3->begin(file, out);                   // Begin MP3 playback
  isAdhanPlaying = true;                   // Set the flag to indicate Adhan is playing
  Serial.println("playing adhan");
}

// Function to play Adhan audio
void playAdhan() {
  if (mp3->isRunning() && !mp3->loop()) {  // Check if MP3 is still playing
    mp3->stop();                           // Stop MP3 playback if done
    isAdhanPlaying = false;                // Reset playback flag
    Serial.println("adhan ended");
  }
}

// Generic function to handle Adhan timing, blinking, and audio playback
// Parameters:
// - now: current DateTime object
// - timingsArray: pointer to array of Adhan times (in minutes)
// - count: number of timings in the array
// - debug: optional flag to print debug messages (default is false)
void handleAdhanTimings(DateTime now, int* timingsArray, int count, bool debug = false) {
  int currentMinutes = now.hour() * 60 + now.minute();  // Convert current time to minutes
  bool adhanTriggered = false;                          // Flag to check if it's time for Adhan

  // Loop through each Adhan timing
  for (int i = 0; i < count; i++) {
    // If the current time matches any Adhan timing and it's within the first 5 seconds
    if (currentMinutes == timingsArray[i] && now.second() < 5) {
      adhanTriggered = true;  // Set the trigger flag
      break;                  // Exit the loop after finding a match
    }
  }

  // Handle blinking logic
  if (adhanTriggered) {
    if (!isBlinking) {    // Only update if blinking wasn't already active
      isBlinking = true;  // Start blinking

      // Optional debug output
      if (debug) {
        Serial.println("Blink True");
        Serial.printf("Hour: %d, Minute: %d, Second: %d\n", now.twelveHour(), now.minute(), now.second());
      }
    }
  } else {
    isBlinking = false;  // Stop blinking if not in Adhan time
  }

  // Handle Adhan audio playback
  if (adhanTriggered || isAdhanPlaying) {
    if (!isAdhanPlaying) {
      loadfile();  // Load MP3 file if not already loaded
    }
    playAdhan();  // Play the Adhan audio
  }
}

// Function to display the current time on the LED matrix
void displayCurrentTime(DateTime now) {
  int hour = now.twelveHour();  // Get the current hour in 12-hour format
  int minute = now.minute();    // Get the current minute

  // Clear the display at 1:01:01 to reset it
  if (hour == 1 && minute == 1 && now.second() == 0) {
    lc.clearMatrix();
  }

  // Display hour and minute on LED matrix
  if (hour < 10) {
    lc.setDigit(0, 1, hour, false);  // Display the ones digit of the hour
  } else {
    lc.setDigit(0, 0, hour / 10, false);  // Display the tens digit of the hour
    lc.setDigit(0, 1, hour % 10, false);  // Display the ones digit of the hour
  }
  lc.setDigit(0, 2, minute / 10, false);  // Display the tens digit of the minute
  lc.setDigit(0, 3, minute % 10, false);  // Display the ones digit of the minute
}

// Function to blink the LED display during Adhan time
// Called repeatedly inside the main loop when isBlinking is true
void blinkDisplay(DateTime now) {
  // Check if 500ms has passed since the last toggle (for blinking effect)
  if (millis() - lastBlinkTime >= 450) {
    lastBlinkTime = millis();  // Update the last blink time

    if (isLEDOn) {
      lc.clearMatrix();        // Turn off the display
      Serial.println("OFF");   // Debug print
    } else {
      displayCurrentTime(now); // Turn display back on by showing time
      Serial.println("ON");    // Debug print
    }

    isLEDOn = !isLEDOn;  // Toggle the LED state for next time
  }
}

// Setup function for initialization
void setup() {
  Serial.begin(115200);                         // Start serial communication
  rtc.begin();                                  // Initialize RTC
  lc.setIntensity(8);                           // Set LED matrix brightness
  lc.clearMatrix();                             // Clear LED matrix
  SD.begin(CHIP_SELECT);                        // Initialize SD card
  out = new AudioOutputI2S();                   // Set up audio output
  out->SetPinout(I2S_BCLK, I2S_LRC, I2S_DOUT);  // Configure I2S pins
  mp3 = new AudioGeneratorMP3();                // Create MP3 generator object
}

// Main loop for continuous execution
void loop() {
  DateTime now = rtc.now();  // Get current time from RTC
  char newDate[11];
  sprintf(newDate, "%02d-%02d-%04d", now.day(), now.month(), now.year());

  // Fetch new Adhan timings if the date has changed
  if (strcmp(newDate, lastFetchDate) != 0) {
    connectWiFi();                   // Connect to WiFi
    fetchTimings(newDate);           // Fetch Adhan timings for the current date
    syncTime();                      // Sync the time with the NTP server
    strcpy(lastFetchDate, newDate);  // Update the last fetched date
    WiFi.disconnect(true);           // Disconnect WiFi to save resources
    WiFi.mode(WIFI_OFF);             // Turn off WiFi
    if (WiFi.status() == WL_CONNECTED) {
      Serial.println("Failed to disconnect WiFi");
    } else {
      Serial.println("WiFi disconnected successfully");
    }
  }
  // Handle Adhan timing and trigger related actions
  handleAdhanTimings(now, adhanTimings, maxAdhanTimings, true);
  handleAdhanTimings(now, testTimings, sizeof(testTimings) / sizeof(testTimings[0]), true);

  Blink for the first 5 seconds, then display normal time
  if (isBlinking) {
    blinkDisplay(now);  // Handle blinking effect
  } else {
    displayCurrentTime(now);  // Display the current time
  }
  displayCurrentTime(now);  // Display the current time

}
