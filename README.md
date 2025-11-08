# EcoPoint - Smart Trash Bin Hardware 🗑️📊

**Hackathon:** Łódź Future City  
**Project:** EcoPoint  
**Repository:** Hardware/Firmware Component

## 🎯 Overview

EcoPoint is a smart trash bin system designed to monitor and track waste disposal in real-time. This repository contains the firmware for ESP8266-based hardware that uses ultrasonic sensors to detect trash deposits and synchronizes data with Firebase Realtime Database.

The system helps cities and organizations:
- Track waste disposal patterns
- Monitor bin usage in real-time
- Optimize waste collection routes
- Encourage eco-friendly behavior through gamification

## 🔧 Hardware Components

- **Microcontroller:** ESP8266 (NodeMCU v2)
- **Ultrasonic Sensor:** HC-SR04 (or compatible)
  - Trigger Pin: GPIO 12 (D6)
  - Echo Pin: GPIO 14 (D5)
- **Power Supply:** USB or external 5V power source
- **WiFi:** Built-in ESP8266 WiFi module

## 📋 Features

- ✅ **Real-time Trash Detection:** Ultrasonic sensor monitors when items are dropped into the bin
- ✅ **Session Management:** Start/stop tracking sessions remotely via Firebase
- ✅ **Debounce Logic:** Prevents false counts with 300ms debounce timing
- ✅ **Auto-Session Timeout:** Sessions automatically end after 20 seconds of inactivity
- ✅ **Cloud Synchronization:** Real-time data sync with Firebase Realtime Database
- ✅ **WiFi Connectivity:** Automatic WiFi connection and reconnection
- ✅ **Memory Optimization:** Includes heap monitoring for stable operation

## 🚀 Getting Started

### Prerequisites

- [PlatformIO](https://platformio.org/) installed (via VS Code extension or CLI)
- ESP8266 NodeMCU v2 board
- HC-SR04 ultrasonic sensor
- WiFi network credentials
- Firebase project with Realtime Database enabled

### Hardware Setup

1. **Connect the Ultrasonic Sensor:**
   - VCC → 5V
   - GND → GND
   - Trig → D6 (GPIO 12)
   - Echo → D5 (GPIO 14)

2. **Connect the ESP8266:** 
   - Connect via USB to your computer for programming
   - Ensure proper power supply (5V, min 500mA recommended)

### Software Setup

1. **Clone the Repository:**
   ```bash
   git clone https://github.com/Better-Then-Never/lodz-trash-bin_hardware.git
   cd lodz-trash-bin_hardware
   ```

2. **Configure Credentials:**
   
   Edit `src/secrets.h` with your credentials:
   ```cpp
   #define WIFI_SSID "your-wifi-ssid"
   #define WIFI_PASSWORD "your-wifi-password"
   #define Web_API_KEY "your-firebase-api-key"
   #define DATABASE_URL "your-firebase-database-url"
   #define USER_EMAIL "your-firebase-user@example.com"
   #define USER_PASS "your-firebase-password"
   ```

   **⚠️ Security Note:** Never commit `secrets.h` with real credentials to version control!

3. **Install Dependencies:**
   ```bash
   pio lib install
   ```

4. **Build and Upload:**
   ```bash
   pio run --target upload
   ```

5. **Monitor Serial Output:**
   ```bash
   pio device monitor
   ```

## 📊 Firebase Database Structure

The device expects the following database structure:

```json
{
  "BinsData": {
    "<USER_UID>": {
      "isSessionStarted": false,
      "trashUnitsCounter": 0
    }
  }
}
```

- **`isSessionStarted`**: Controls whether the bin is actively tracking (set to `"true"` or `"false"`)
- **`trashUnitsCounter`**: Current count of trash items deposited in the session

## 🔄 How It Works

1. **Session Start:**
   - Set `isSessionStarted` to `"true"` in Firebase
   - Device resets counter to 0 and begins monitoring

2. **Trash Detection:**
   - Ultrasonic sensor measures distance every 50ms
   - When distance < 12cm, trash is detected
   - Counter increments and syncs to Firebase
   - Debounce prevents duplicate counts

3. **Session End:**
   - Automatically ends after 20 seconds of no activity
   - Can be manually ended by setting `isSessionStarted` to `"false"`
   - Final count is preserved in Firebase

## ⚙️ Configuration Parameters

You can adjust these constants in `main.cpp`:

| Parameter | Default | Description |
|-----------|---------|-------------|
| `SOUND_VELOCITY` | 0.034 cm/μs | Speed of sound for distance calculation |
| `trigPin` | 12 | GPIO pin for ultrasonic trigger |
| `echoPin` | 14 | GPIO pin for ultrasonic echo |
| `checkInterval` | 250ms | How often to check Firebase for session updates |
| `sensorCheckInterval` | 50ms | How often to read the ultrasonic sensor |
| `timeBeforeSessionEnds` | 20000ms | Inactivity timeout for auto-ending sessions |
| `distanceToBinWall` | 12cm | Maximum distance to detect trash |
| `dropDebounceMs` | 300ms | Debounce time between trash detections |

## 🐛 Debugging

The firmware includes extensive serial logging:

```cpp
Serial.begin(115200);
```

Monitor output shows:
- WiFi connection status
- Firebase authentication
- Session state changes
- Trash detection events
- Distance measurements
- Memory (heap) usage

## 📦 Dependencies

- **[FirebaseClient](https://github.com/mobizt/FirebaseClient)** v2.2.2 - Firebase integration for ESP8266
- **ESP8266WiFi** - Built-in WiFi library
- **Arduino Framework** - Core Arduino functionality

## 🔐 Security Best Practices

1. **Never commit credentials:** Add `secrets.h` to `.gitignore`
2. **Use Firebase Security Rules:** Restrict database access appropriately
3. **Create dedicated service account:** Don't use personal Firebase credentials
4. **Secure WiFi:** Use WPA2/WPA3 encrypted networks

## 🤝 Contributing

This project was developed for the Łódź Future City hackathon. Contributions, issues, and feature requests are welcome!

1. Fork the repository
2. Create your feature branch (`git checkout -b feature/AmazingFeature`)
3. Commit your changes (`git commit -m 'Add some AmazingFeature'`)
4. Push to the branch (`git push origin feature/AmazingFeature`)
5. Open a Pull Request

## 📝 License

This project is part of the EcoPoint hackathon submission for Łódź Future City.

## 👥 Team

**Better Then Never**

- Hardware & Firmware Development
- IoT Integration
- Real-time Data Synchronization

## 🏆 Acknowledgments

- **Łódź Future City Hackathon** organizers
- FirebaseClient library by Mobizt
- PlatformIO community

## 📞 Support

For questions or issues related to this hardware component, please open an issue in this repository.

---

**Made with ❤️ for a cleaner Łódź**
