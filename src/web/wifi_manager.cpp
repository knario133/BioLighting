#include "wifi_manager.h"
#include "../config.h"
#include <WiFi.h>

WiFiManager::WiFiManager(Storage& storage) : _storage(storage) {
    // El modo se determinará dinámicamente en `begin`.
    _currentMode = WiFiMode::OFF;
}

void WiFiManager::begin() {
    // 1. Configurar siempre el modo AP_STA
    WiFi.mode(WIFI_AP_STA);

    // 2. Iniciar el Punto de Acceso (AP)
    String ssid_ap;
    uint8_t mac[6];
    WiFi.macAddress(mac);
    char mac_suffix[5];
    sprintf(mac_suffix, "%02X%02X", mac[4], mac[5]);
    ssid_ap = "BioLighting-AP-" + String(mac_suffix);

    // Inicia el AP sin contraseña. La IP por defecto es 192.168.4.1
    WiFi.softAP(ssid_ap.c_str());
    Serial.printf("AP Mode started. SSID: %s, IP: %s\n", ssid_ap.c_str(), WiFi.softAPIP().toString().c_str());

    // 3. Intentar conectar a la red guardada (STA)
    String ssid_sta, pass_sta;
    if (_storage.loadWifiCredentials(ssid_sta, pass_sta)) {
        Serial.printf("Attempting to connect to saved WiFi: %s\n", ssid_sta.c_str());
        WiFi.begin(ssid_sta.c_str(), pass_sta.c_str());

        // Espera no bloqueante (la conexión real se gestiona en segundo plano)
        // El estado se comprobará con isConnected()
    } else {
        Serial.println("No saved STA credentials. Running in AP mode only.");
    }
}

bool WiFiManager::isConnected() {
    return WiFi.status() == WL_CONNECTED;
}

WiFiMode WiFiManager::getMode() {
    // La lógica de modo ahora es dinámica
    if (WiFi.status() == WL_CONNECTED) {
        return WiFiMode::STA;
    }
    // Si no está conectado como STA, consideramos que está en modo AP
    // ya que el AP siempre está activo.
    return WiFiMode::AP;
}

String WiFiManager::getStaIp() {
    if (isConnected()) {
        return WiFi.localIP().toString();
    }
    return ""; // Devuelve cadena vacía si no está conectado
}

String WiFiManager::getApSsid() {
    return WiFi.softAPSSID();
}

String WiFiManager::getApIp() {
    return WiFi.softAPIP().toString();
}

void WiFiManager::forceApMode() {
    // Simplemente borra las credenciales y reinicia.
    // Al arrancar, no encontrará credenciales y permanecerá en modo AP.
    _storage.resetWifiCredentials();
    Serial.println("WiFi credentials reset. Restarting to apply changes.");
    delay(1000);
    ESP.restart();
}
