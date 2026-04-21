#ifndef WIFI_MODULE_H
#define WIFI_MODULE_H

#include <Arduino.h>
#include <WiFi.h>
//#include <config.h>

// ======================= КОНСТАНТЫ =======================
#define MAX_NETWORKS 15
#define LIST_X 85
#define LIST_Y 40
#define LIST_W 190
#define LIST_H 160
#define LIST_ITEM_H 20

// ======================= СТРУКТУРЫ =======================
struct WiFiNetwork {
  char ssid[33];
  int32_t rssi;
  uint8_t encryptionType;
};

// ======================= EXTERN ПЕРЕМЕННЫЕ =======================
// Определены в wifi_module.cpp, доступны извне
extern WiFiNetwork wifiNetworks[MAX_NETWORKS];
extern int wifiNetworkCount;
extern int wifiScrollOffset;
extern int selectedNetwork;
extern bool wifiConnected;
extern String connectedSSID;
//extern char StandartWiFiPass[30];
extern char wifiTargetSSID[33];
extern bool wifiWaitingForPassword;
extern bool wifiScanning;

// ======================= ФУНКЦИИ =======================
void wifiInit();
void scanWiFiNetworks();
void drawWiFiScreen();
void handleWiFiTouch(uint16_t x, uint16_t y);
void connectToWiFi(const char* ssid, const char* password);
void wifiDisconnect();
bool wifiIsConnected();

#endif
