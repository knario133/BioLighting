# BioLighting ESP32 Firmware - v2 (Multi-Language)

This repository contains the PlatformIO firmware for an ESP32-based RGB lighting controller. This version includes a redesigned user interface with multi-language support and a "glassmorphism" visual style.

## Features

- **RGB Control**: Control a strip of WS2811 LEDs using the FastLED library.
- **Bilingual Physical UI**: Adjust Red, Green, Blue, and Intensity values using a 16x2 I2C LCD and a rotary encoder. The UI supports both Spanish and English, and the preference is saved.
- **Redesigned Web UI**: A clean, responsive "glassmorphism" web interface with sliders, preset buttons, and a language selector (Spanish/English).
- **Interactive Alerts**: Uses SweetAlert2 for user-friendly confirmation dialogs.
- **REST API**: A simple API to get and set the light's state programmatically, including presets.
- **WiFi Manager**: On first boot, the device starts in AP mode with a captive portal to easily configure your WiFi credentials.
- **Persistence**: Light settings, language preference, and WiFi credentials are saved to non-volatile storage (NVS) and restored on reboot.

## Hardware Requirements

- **Board**: ESP32 development board (e.g., ESP32-DevKitC)
- **LEDs**: WS2811 RGB LED strip
- **Display**: 16x2 I2C LCD Display
- **Input**: Rotary Encoder with a push-button

### Pinout

- **DATA_PIN** (for WS2811 LEDs): `GPIO 25`
- **I2C_SDA** (for LCD): `GPIO 21`
- **I2C_SCL** (for LCD): `GPIO 22`
- **ENC_DT** (Rotary Encoder DT): `GPIO 18`
- **ENC_CLK** (Rotary Encoder CLK): `GPIO 5`
- **ENC_SW** (Rotary Encoder Switch): `GPIO 19`

## Library Dependencies

The project is built on PlatformIO and relies on the following libraries:

- `fastled/FastLED`
- `bblanchon/ArduinoJson`
- `me-no-dev/ESP Async WebServer`
- `esphome/AsyncTCP`
- `mathertel/RotaryEncoder`
- `marcoschwartz/LiquidCrystal_I2C`

These are automatically managed by PlatformIO via the `platformio.ini` file.

## How to Build and Flash

1.  **Install PlatformIO**: Make sure you have [PlatformIO IDE for VSCode](https://platformio.org/platformio-ide) installed.
2.  **Clone the Repository**:
    ```bash
    git clone <repository_url>
    cd <repository_folder>
    ```
3.  **Build**: Open the project in VSCode with the PlatformIO extension. In the PlatformIO toolbar, click on "Build".
4.  **Upload**: Connect your ESP32 board to your computer. In the PlatformIO toolbar, click on "Upload".
5.  **Upload Filesystem Image**: The web interface files (`/ui_web`) need to be uploaded to the ESP32's LittleFS filesystem. In the PlatformIO toolbar, under the "Project Tasks" for your environment, click on "Upload Filesystem Image". This step is crucial for the web UI to work.

## Usage

### First-Time Setup (WiFi Configuration)

1.  On the first boot, the device will start in Access Point (AP) mode with the SSID **`BioShacker_Conf`**.
2.  Connect to this network. A captive portal should open automatically. If not, navigate to `http://192.168.4.1`.
3.  Enter your home WiFi credentials and save. The device will restart and connect to your network.

### Physical UI (LCD + Encoder)

-   **Turn the Encoder**: Adjusts the value of the currently selected menu (R, G, B, Intensity) or selects the language.
-   **Press the Encoder Button**: Cycles through the seven menus: Red -> Green -> Blue -> Intensity -> Language -> WiFi Connect/Disconnect -> WiFi Change Network.
-   **WiFi Menus**: Allows you to connect/disconnect from the saved network or clear credentials and restart in AP mode.
-   Settings (color and language) are saved automatically.

### Web UI

1.  Once connected to your WiFi, the device's IP address will be shown on the LCD.
2.  Navigate to this IP in a web browser.
3.  The interface allows you to:
    -   Select your preferred language (Español/English).
    -   Adjust R, G, B, and Intensity with sliders.
    -   Apply presets (`Warm`, `Cool`, `Sunset`).
    -   Reset the color to a default state.
    -   All actions will prompt for confirmation using a dialog box.

### REST API

The following endpoints are available:

#### Get Light State

-   **Endpoint**: `GET /api/light`
-   **Response**: `{"r": <0-255>, "g": <0-255>, "b": <0-255>, "intensity": <0-100>}`

#### Set Light State

-   **Endpoint**: `POST /api/light`
-   **Body**: JSON with the same structure as the GET response.
-   **Response**: `200 OK` with the new state. `400 Bad Request` if data is invalid.

#### Get Presets

-   **Endpoint**: `GET /api/presets`
-   **Response**: `["warm", "cool", "sunset"]`

#### Apply a Preset

-   **Endpoint**: `POST /api/preset/{name}` (e.g., `/api/preset/warm`)
-   **Response**: `200 OK` with the new state. `404 Not Found` if the preset is invalid.

#### Reset WiFi

-   **Endpoint**: `POST /api/wifi/reset`
-   **Description**: Clears saved WiFi credentials. The device must be rebooted manually to re-enter AP mode.
-   **Response**: `200 OK` with a plain text confirmation.

#### Get WiFi Status

-   **Endpoint**: `GET /api/wifi/status`
-   **Description**: Retrieves the current WiFi connection status.
-   **Response**:
    ```json
    {
      "wifi": true,
      "mode": "STA",
      "ip": "192.168.1.123",
      "ssid": "MyNetwork",
      "rssi": -58
    }
    ```
