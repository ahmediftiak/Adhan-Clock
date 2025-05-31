# 🕌 Adhan Timing and Display System

## Overview

This project is an **Adhan Timing and Display System** powered by an **ESP32** microcontroller. It combines:

- Accurate timekeeping via **DS3231 RTC** and **NTP**
- Automatic fetching of daily prayer times via **AlAdhan API**
- **LED matrix display** for time visualization
- **MP3 Adhan playback** through **I2S audio output**
- Offline operation once initial sync is done

Ideal for use in mosques, homes, or embedded Islamic applications.

---

## 🧰 Hardware Requirements

- **ESP32 Development Board**
- **DS3231 RTC Module**
- **MAX7219 8x8 LED Matrix Display**
- **MicroSD Card Module + MicroSD card** (FAT32 formatted)
- **Speaker + I2S amplifier (e.g. MAX98357A)**
- **Jumper wires / Breadboard**
- **Power supply** (USB or battery)

---

## 💻 Software Requirements

- **Arduino IDE** with ESP32 board support
- Required Libraries:
  - `RTClib` – for RTC access
  - `WiFi` – for internet connectivity
  - `HTTPClient` – for API requests
  - `ArduinoJson` – for parsing prayer time data
  - `LedController` – for MAX7219 LED control
  - `AudioFileSourceSD`, `AudioGeneratorMP3`, `AudioOutputI2S` – for MP3 playback via I2S

---

## 🔌 Wiring

| Component      | ESP32 Pin        |
|----------------|------------------|
| **DS3231 RTC** | SDA → GPIO 21<br>SCL → GPIO 22 |
| **SD Module**  | CS → GPIO 5<br>MOSI → GPIO 23<br>MISO → GPIO 19<br>SCK → GPIO 18 |
| **LED Matrix** | DIN → GPIO 13<br>CLK → GPIO 14<br>CS → GPIO 15 |
| **Speaker (I2S)** | LRC → GPIO 2<br>BCLK → GPIO 3<br>DOUT → GPIO 4 |

_Note: Adjust pins in code if using different GPIOs._

---

## 🗂️ Setup Instructions

### 1. Library Installation

Open Arduino IDE, then:

1. Go to **Sketch → Include Library → Manage Libraries**
2. Install the following:
   - `RTClib`
   - `WiFi`
   - `HTTPClient`
   - `ArduinoJson`
   - `LedController`
   - `ESP8266Audio` (includes `AudioGeneratorMP3`, etc.)

### 2. Prepare SD Card

- Format the SD card as **FAT32**
- Place your Adhan file in root directory as `a1.mp3`

### 3. Configure WiFi and Location

Edit in your Arduino sketch:

```cpp
const char* ssid = "Your_SSID";
const char* password = "Your_PASSWORD";

String city = "New York";
String country = "USA";

const long gmtOffset_sec = -18000;
const int daylightOffset_sec = 3600;

## 🚀 How It Works

### On Startup:
- Connects to WiFi  
- Syncs time using NTP  
- Fetches prayer times from **AlAdhan API**  
- Stores them in memory for the day  
- Disconnects WiFi to save power  

### During Operation:
- Continuously displays time on LED matrix  
- Compares current time with prayer schedule  
- When a prayer time matches:
  - 🔁 Blinks LED display  
  - 🔊 Plays Adhan from SD card  

---

## 🧠 Core Functions

| Function               | Purpose                                      |
|------------------------|----------------------------------------------|
| `connectWiFi()`        | Connects ESP32 to local WiFi                 |
| `syncTime()`           | Syncs time using NTP and updates RTC         |
| `fetchTimings(date)`   | Fetches prayer times from AlAdhan API        |
| `timeToMinutes("HH:MM")` | Converts time string to minutes            |
| `handleAdhanTimings()` | Checks for Adhan time and triggers playback  |
| `playAdhan()`          | Plays Adhan MP3 via I2S                      |
| `displayCurrentTime()` | Displays the time on LED matrix              |
| `blinkDisplay()`       | Makes matrix blink during Adhan              |

## License

This project is licensed under the MIT License.