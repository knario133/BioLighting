# BioLighting ESP32 Firmware - v2 (Multi-idioma)

Este repositorio contiene el firmware en PlatformIO para un controlador de iluminación RGB basado en ESP32. Esta versión incluye una interfaz de usuario rediseñada con soporte multi-idioma y un estilo visual "glassmorphism".

## Características

- **Control RGB**: Controla una tira de LEDs WS2811 utilizando la librería FastLED.
- **UI Física Bilingüe**: Ajusta los valores de Rojo, Verde, Azul e Intensidad usando una pantalla LCD I2C de 16x2 y un codificador rotatorio. La interfaz soporta tanto español como inglés, y la preferencia se guarda.
- **UI Web Rediseñada**: Una interfaz web limpia y responsiva con estilo "glassmorphism", sliders, botones de presets y un selector de idioma (español/inglés).
- **Alertas Interactivas**: Usa SweetAlert2 para diálogos de confirmación amigables.
- **API REST**: Una API sencilla para obtener y establecer el estado de la luz de forma programática, incluyendo presets.
- **Gestor de WiFi**: En el primer arranque, el dispositivo inicia en modo AP con un portal cautivo para configurar fácilmente tus credenciales de WiFi.
- **Persistencia**: Los últimos ajustes de luz, la preferencia de idioma y las credenciales de WiFi se guardan en el almacenamiento no volátil (NVS) y se restauran al reiniciar.

## Requisitos de Hardware

- **Placa**: Placa de desarrollo ESP32 (ej. ESP32-DevKitC)
- **LEDs**: Tira de LEDs RGB WS2811
- **Pantalla**: Pantalla LCD I2C 16x2
- **Entrada**: Codificador rotatorio (Rotary Encoder) con pulsador

### Pinout (Conexiones)

- **DATA_PIN** (para LEDs WS2811): `GPIO 25`
- **I2C_SDA** (para LCD): `GPIO 21`
- **I2C_SCL** (para LCD): `GPIO 22`
- **ENC_DT** (Rotary Encoder DT): `GPIO 18`
- **ENC_CLK** (Rotary Encoder CLK): `GPIO 5`
- **ENC_SW** (Rotary Encoder Pulsador): `GPIO 19`

## Dependencias de Librerías

El proyecto está construido sobre PlatformIO y depende de las siguientes librerías:

- `fastled/FastLED`
- `bblanchon/ArduinoJson`
- `me-no-dev/ESP Async WebServer`
- `esphome/AsyncTCP`
- `mathertel/RotaryEncoder`
- `marcoschwartz/LiquidCrystal_I2C`

Estas son gestionadas automáticamente por PlatformIO a través del archivo `platformio.ini`.

## Cómo Compilar y Flashear

1.  **Instalar PlatformIO**: Asegúrate de tener instalado [PlatformIO IDE para VSCode](https://platformio.org/platformio-ide).
2.  **Clonar el Repositorio**:
    ```bash
    git clone <url_del_repositorio>
    cd <carpeta_del_repositorio>
    ```
3.  **Compilar**: Abre el proyecto en VSCode con la extensión de PlatformIO. En la barra de herramientas de PlatformIO, haz clic en "Build".
4.  **Subir Firmware**: Conecta tu placa ESP32 a tu ordenador. En la barra de herramientas de PlatformIO, haz clic en "Upload".
5.  **Subir Sistema de Archivos**: Los archivos de la interfaz web (`/ui_web`) deben subirse al sistema de archivos LittleFS del ESP32. En la barra de herramientas de PlatformIO, bajo "Project Tasks" para tu entorno, haz clic en "Upload Filesystem Image". Este paso es crucial para que la UI web funcione.

## Uso

### Primera Configuración (Configuración de WiFi)

1.  En el primer arranque, el dispositivo iniciará en modo Punto de Acceso (AP) con el SSID **`BioShacker_Conf`**.
2.  Conéctate a esta red. Un portal cautivo debería abrirse automáticamente. Si no, navega a `http://192.168.4.1`.
3.  Introduce las credenciales de tu red WiFi local y guarda. El dispositivo se reiniciará y se conectará a tu red.

### UI Física (LCD + Encoder)

-   **Girar el Encoder**: Ajusta el valor del menú actual (R, G, B, Intensidad) o selecciona el idioma.
-   **Pulsar el Encoder**: Cambia entre los siete menús: Rojo -> Verde -> Azul -> Intensidad -> Idioma -> WiFi Conectar/Desconectar -> Cambiar Red WiFi.
-   **Menús de WiFi**: Te permite conectar/desconectar de la red guardada o borrar las credenciales y reiniciar en modo AP.
-   Los ajustes (color e idioma) se guardan automáticamente.

### UI Web

1.  Una vez que el dispositivo esté conectado a tu WiFi, la pantalla LCD mostrará su dirección IP.
2.  Navega a esa IP en un navegador web.
3.  La interfaz te permite:
    -   Seleccionar tu idioma preferido (Español/English).
    -   Ajustar R, G, B e Intensidad con sliders.
    -   Aplicar presets (`Warm`, `Cool`, `Sunset`).
    -   Restablecer el color a un estado por defecto.
    -   Todas las acciones pedirán confirmación usando un diálogo.

### API REST

Los siguientes endpoints están disponibles:

#### Obtener Estado de la Luz

-   **Endpoint**: `GET /api/light`
-   **Respuesta**: `{"r": <0-255>, "g": <0-255>, "b": <0-255>, "intensity": <0-100>}`

#### Establecer Estado de la Luz

-   **Endpoint**: `POST /api/light`
-   **Cuerpo (Body)**: JSON con la misma estructura que la respuesta del GET.
-   **Respuesta**: `200 OK` con el nuevo estado. `400 Bad Request` si los datos son inválidos.

#### Obtener Presets

-   **Endpoint**: `GET /api/presets`
-   **Respuesta**: `["warm", "cool", "sunset"]`

#### Aplicar un Preset

-   **Endpoint**: `POST /api/preset/{name}` (ej., `/api/preset/warm`)
-   **Respuesta**: `200 OK` con el nuevo estado. `404 Not Found` si el preset no es válido.

#### Resetear WiFi

-   **Endpoint**: `POST /api/wifi/reset`
-   **Descripción**: Borra las credenciales de WiFi guardadas en NVS. El dispositivo debe ser reiniciado manualmente para reingresar al modo AP.
-   **Respuesta**: `200 OK` con una confirmación en texto plano.

#### Obtener Estado del WiFi

-   **Endpoint**: `GET /api/wifi/status`
-   **Descripción**: Obtiene el estado actual de la conexión WiFi.
-   **Respuesta**:
    ```json
    {
      "wifi": true,
      "mode": "STA",
      "ip": "192.168.1.123",
      "ssid": "MiRed",
      "rssi": -58
    }
    ```
