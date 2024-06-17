# Adhan Timing and Display System

## Overview

This project is an Adhan Timing and Display System designed using an Arduino, an RTC module, WiFi connectivity, and an LED matrix. The system fetches daily prayer times from an online API and displays the current time on an LED matrix. At specified prayer times (Adhan), it plays an MP3 file and blinks the LED matrix display.

## Hardware Requirements

- Arduino board (e.g., ESP32)
- RTC module (e.g., DS3231)
- SD card module
- LED matrix display
- Audio components (I2S compatible)
- WiFi connectivity
- SD card with MP3 file

## Software Requirements

- Arduino IDE
- Libraries:
  - `RTClib`
  - `WiFi`
  - `HTTPClient`
  - `ArduinoJson`
  - `LedController`
  - `AudioFileSourceSD`
  - `AudioGeneratorMP3`
  - `AudioOutputI2S`

## Installation

1. **Install Required Libraries**:
   - Open Arduino IDE: `Sketch -> Include Library -> Manage Libraries`
   - Install libraries listed above.
2. **Set Up Hardware**:
   - Connect the RTC, SD card module, LED matrix, and audio components to the Arduino.
3. **Prepare SD Card**:
   - Copy `a1.mp3` to the SD card's root directory.

## Configuration

1. **WiFi Configuration**:
   - Open the sketch and enter your WiFi credentials:
     ```cpp
     const char* ssid = "your_SSID";
     const char* password = "your_PASSWORD";
     ```

## Usage

1. **Upload the Code**:
   - Connect Arduino, open the sketch, select the board and port, and upload.
2. **Run the System**:
   - Connects to WiFi and fetches prayer times.
   - Displays current time on LED matrix.
   - Plays Adhan MP3 and blinks LED matrix at prayer times.

## Code Overview

### Main Loop

1. **Date Change Check**:
   - The system checks if the date has changed since the last API fetch. If so, it fetches new prayer times for the current date.

2. **Display Current Time**:
   - The current time is continuously displayed on the LED matrix.

3. **Adhan Time Check**:
   - The system checks if the current time matches any of the fetched prayer times. If a match is found:
     - The LED matrix blinks.
     - The Adhan MP3 file is played.

### Function Details

- `setup()`: Initializes the system, sets up the RTC, connects to WiFi, and initializes the SD card and audio components.
- `loop()`: Continuously checks the current time, fetches new prayer times if the date changes, checks for Adhan times, and updates the LED matrix display.
- `printTime(DateTime now)`: Prints the current time to the serial monitor.
- `connectWiFi()`: Connects the system to the WiFi network.
- `fetchTimings(String date)`: Fetches prayer timings from the API based on the given date.
- `timeToMinutes(const char* timeStr)`: Converts a time string to minutes.
- `checkAdhanTimings()`: Checks if it's time for a prayer call (Adhan).
- `displayCurrentTime(DateTime now)`: Displays the current time on the LED matrix.
- `blinkDisplay()`: Blinks the LED matrix display.
- `playAdhan()`: Plays the Adhan MP3.
- `loadfile()`: Loads the MP3 file.

## License

This project is licensed under the MIT License.

---

## Troubleshooting

- **WiFi Issues**: Check SSID/password and network availability.
- **RTC Issues**: Ensure RTC module is connected and library is installed.
- **Audio Issues**: Check audio component connections and MP3 file location.

