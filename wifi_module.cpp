#include "wifi_module.h"
#include <TFT_eSPI.h>
#include <config.h>
// Объявление внешнего TFT объекта (из main)
extern TFT_eSPI tft;

// ======================= ОПРЕДЕЛЕНИЯ ПЕРЕМЕННЫХ =======================
WiFiNetwork wifiNetworks[MAX_NETWORKS];
int wifiNetworkCount = 0;
int wifiScrollOffset = 0;
int selectedNetwork = -1;
bool wifiConnected = false;
String connectedSSID = "";
extern char StandartWiFiPass[32];
char wifiTargetSSID[33] = "";
bool wifiWaitingForPassword = false;
bool wifiScanning = false;

// ======================= ВСПОМОГАТЕЛЬНЫЕ ФУНКЦИИ =======================
// Объявления внешних функций из main (drawScanlines, drawScanlinesButtons)
extern void drawScanlines();
extern void drawScanlinesButtons(int16_t xS, int16_t yS, int16_t hS, int16_t wS);
extern void drawTabButtons();
extern void drawButtonsScreen4();

// RGB LED пины (из config)
//extern const int LED_R;
//extern const int LED_G;
//extern const int LED_B;

// ======================= ИНИЦИАЛИЗАЦИЯ =======================
void wifiInit() {
  WiFi.mode(WIFI_OFF);
  delay(100);
  WiFi.mode(WIFI_STA);
  String hostname = "Vaul-Tec_PipBoy_" + String(ESP.getEfuseMac(), HEX);
  hostname = hostname.substring(0, 20);
  WiFi.setHostname(hostname.c_str());
}

// ======================= СКАНИРОВАНИЕ =======================
void scanWiFiNetworks() {
  if (wifiScanning) return;
  wifiScanning = true;
  
  // Окно сканирования
  tft.fillRect(40, 80, 240, 80, TFT_BLACK);
  tft.drawRect(40, 80, 240, 80, TFT_GREEN);
  tft.setTextColor(TFT_GREEN);
  tft.setTextDatum(MC_DATUM);
  tft.setTextSize(1);
  tft.drawString("SCANNING...", 160, 120);
  
  WiFi.mode(WIFI_STA);
  delay(100);
  
  int n = WiFi.scanNetworks();
  wifiNetworkCount = (n > MAX_NETWORKS) ? MAX_NETWORKS : n;
  
  for (int i = 0; i < wifiNetworkCount; i++) {
    strncpy(wifiNetworks[i].ssid, WiFi.SSID(i).c_str(), 32);
    wifiNetworks[i].ssid[32] = '\0';
    wifiNetworks[i].rssi = WiFi.RSSI(i);
    wifiNetworks[i].encryptionType = WiFi.encryptionType(i);
  }
  
  WiFi.scanDelete();
  wifiScanning = false;
  wifiScrollOffset = 0;
  selectedNetwork = -1;
}

// ======================= ОТРИСОВКА ЭКРАНА =======================
void drawWiFiScreen() {
  tft.fillScreen(TFT_BLACK);
  drawScanlines();
  
  // Кнопки меню
  drawButtonsScreen4();
  
  // Статус подключения
  tft.setTextColor(TFT_GREEN);
  tft.setTextSize(1);
  if (wifiConnected) {
    tft.setCursor(5, LIST_Y - 25);
    tft.print("STATUS: OK > ");
    tft.print(connectedSSID);
  } else {
    tft.setCursor(5, LIST_Y - 25);
    tft.print("STATUS: DISCONNECTED");
  }
  
  // Рамка списка
  tft.drawRect(LIST_X, LIST_Y, LIST_W, LIST_H, TFT_GREEN);
  
  // Кнопки скролла (справа от списка)
  // Вверх
  tft.fillRect(LIST_X + LIST_W + 5, LIST_Y, 35, 35, TFT_BLACK);
  tft.drawRect(LIST_X + LIST_W + 5, LIST_Y, 35, 35, TFT_GREEN);
  tft.setTextDatum(MC_DATUM);
  tft.drawString("/\\", LIST_X + LIST_W + 22, LIST_Y + 18);
  
  // Вниз
  tft.fillRect(LIST_X + LIST_W + 5, LIST_Y + LIST_H - 35, 35, 35, TFT_BLACK);
  tft.drawRect(LIST_X + LIST_W + 5, LIST_Y + LIST_H - 35, 35, 35, TFT_GREEN);
  tft.drawString("\\/", LIST_X + LIST_W + 22, LIST_Y + LIST_H - 18);
  
  // Кнопка SCAN (справа в центре)
  tft.fillRect(LIST_X + LIST_W + 5, (LIST_Y + LIST_H)/2, 35, 35, TFT_BLACK);
  tft.drawRect(LIST_X + LIST_W + 5, (LIST_Y + LIST_H)/2, 35, 35, TFT_GREEN);
  tft.setTextDatum(MC_DATUM);
  tft.setTextSize(1);
  tft.drawString("SCAN", LIST_X + LIST_W + 22, (LIST_Y + LIST_H)/2 + 17);
  
  // Кнопка DISCONNECT/CONNECT
  if (wifiConnected) {
    tft.fillRect(LIST_X + LIST_W - 55, LIST_Y - 35, 60, 25, TFT_BLACK);
    tft.drawRect(LIST_X + LIST_W - 55, LIST_Y - 35, 60, 25, TFT_GREEN);
    tft.drawString("DISCONECT", (LIST_X + LIST_W - 55) + 30, (LIST_Y - 35) + 12);
  } else {
    tft.fillRect(LIST_X + LIST_W - 55, LIST_Y - 35, 60, 25, TFT_BLACK);
    tft.drawRect(LIST_X + LIST_W - 55, LIST_Y - 35, 60, 25, TFT_GREEN);
    tft.drawString("CONNECT", (LIST_X + LIST_W - 55) + 30, (LIST_Y - 35) + 12);
  }
  
  // Список сетей
  tft.setTextDatum(TL_DATUM);
  int itemsPerPage = LIST_H / LIST_ITEM_H;
  
  for (int i = 0; i < itemsPerPage; i++) {
    int idx = wifiScrollOffset + i;
    if (idx >= wifiNetworkCount) break;
    
    int y = LIST_Y + 4 + i * LIST_ITEM_H;
    
    // Выделение выбранной сети
    if (idx == selectedNetwork) {
      tft.fillRect(LIST_X + 2, y - 1, LIST_W - 4, LIST_ITEM_H - 2, TFT_GREEN);
      tft.setTextColor(TFT_BLACK);
    } else {
      tft.setTextColor(TFT_GREEN);
    }
    
    // Иконка замка для защищенных сетей
    if (wifiNetworks[idx].encryptionType != WIFI_AUTH_OPEN) {
      tft.drawString("[*]", LIST_X + 8, y);
    }
    
    // SSID (обрезаем до 18 символов)
    char ssidShort[19];
    strncpy(ssidShort, wifiNetworks[idx].ssid, 18);
    ssidShort[18] = '\0';
    tft.drawString(ssidShort, LIST_X + 30, y);
    
    // Уровень сигнала
    int rssi = wifiNetworks[idx].rssi;
    char rssiStr[6];
    sprintf(rssiStr, "%d", rssi);
    tft.drawString(rssiStr, LIST_X + LIST_W - 24, y);
  }
  
  // Полоса прокрутки
  if (wifiNetworkCount > itemsPerPage) {
    int barHeight = (LIST_H * itemsPerPage) / wifiNetworkCount;
    int barY = LIST_Y + (wifiScrollOffset * LIST_H) / wifiNetworkCount;
    tft.fillRect(LIST_X + LIST_W - 4, barY, 3, barHeight, TFT_GREEN);
  }
}

// ======================= ОБРАБОТКА ТАЧА =======================
void handleWiFiTouch(uint16_t x, uint16_t y) {
  // Кнопка скролл вверх
  if (x >= LIST_X + LIST_W + 5 && x <= LIST_X + LIST_W + 40 && 
      y >= LIST_Y && y <= LIST_Y + 35) {
    if (wifiScrollOffset > 0) {
      digitalWrite(LED_B, LOW);
      delay(30);
      digitalWrite(LED_B, HIGH);
      wifiScrollOffset--;
      drawWiFiScreen();
    }
    return;
  }
  
  // Кнопка скролл вниз
  if (x >= LIST_X + LIST_W + 5 && x <= LIST_X + LIST_W + 40 && 
      y >= LIST_Y + LIST_H - 35 && y <= LIST_Y + LIST_H) {
    int itemsPerPage = LIST_H / LIST_ITEM_H;
    if (wifiScrollOffset + itemsPerPage < wifiNetworkCount) {
      digitalWrite(LED_B, LOW);
      delay(30);
      digitalWrite(LED_B, HIGH);
      wifiScrollOffset++;
      drawWiFiScreen();
    }
    return;
  }
  
  // Кнопка SCAN
  if (x >= (LIST_X + LIST_W + 5) && x <= (LIST_X + LIST_W + 40) && 
      y >= (LIST_Y + LIST_H)/2 && y <= (LIST_Y + LIST_H)/2 + 35) {
    digitalWrite(LED_B, LOW);
    delay(30);
    digitalWrite(LED_B, HIGH);
    scanWiFiNetworks();
    drawWiFiScreen();
    return;
  }
  
  // Кнопка DISCONNECT
  if (wifiConnected && x >= LIST_X + LIST_W - 55 && x <= LIST_X + LIST_W && 
      y >= LIST_Y - 35 && y <= LIST_Y - 10) {
    digitalWrite(LED_B, LOW);
    delay(30);
    digitalWrite(LED_B, HIGH);
    wifiDisconnect();
    drawWiFiScreen();
    return;
  }

  // Кнопка CONNECT
  if (!wifiConnected && x >= LIST_X + LIST_W - 55 && x <= LIST_X + LIST_W && 
      y >= LIST_Y - 35 && y <= LIST_Y - 10) {
    digitalWrite(LED_B, LOW);
    delay(30);
    digitalWrite(LED_B, HIGH);
    
    if (strlen(wifiTargetSSID) > 0) {
      if (wifiNetworks[selectedNetwork].encryptionType == WIFI_AUTH_OPEN) {
        connectToWiFi(wifiTargetSSID, "");
      } else {
        wifiWaitingForPassword = true;
        // Вызов клавиатуры через extern функцию
        extern void showKeyboard(const char* placeholder);
        showKeyboard(StandartWiFiPass);
      }
    }
    return;
  }

  // Выбор сети из списка
  if (x >= LIST_X && x <= LIST_X + LIST_W - 5 && y >= LIST_Y && y <= LIST_Y + LIST_H) {
    int relY = y - LIST_Y - 4;
    int idx = (relY / LIST_ITEM_H) + wifiScrollOffset;
    
    if (idx >= 0 && idx < wifiNetworkCount) {
      selectedNetwork = idx;
      strncpy(wifiTargetSSID, wifiNetworks[idx].ssid, 32);
      wifiTargetSSID[32] = '\0';
      drawWiFiScreen();
      delay(200);
    }
  }
}

// ======================= ПОДКЛЮЧЕНИЕ =======================
void connectToWiFi(const char* ssid, const char* password) {
  // Экран подключения
  tft.fillScreen(TFT_BLACK);
  drawScanlines();
  tft.setTextColor(TFT_GREEN);
  tft.setTextDatum(MC_DATUM);
  tft.setTextSize(1);
  tft.fillRect(40, 80, 240, 80, TFT_BLACK);
  tft.drawRect(40, 80, 240, 80, TFT_GREEN);
  tft.drawString("CONNECTING TO:", 160, 90);
  tft.drawString(ssid, 160, 110);
  tft.setTextColor(tft.color565(0, 35, 0));
  tft.drawString(password, 160, 120);

  tft.setTextColor(TFT_GREEN);
  WiFi.begin(ssid, password);
  
  int dots = 0;
  unsigned long startAttempt = millis();
  
  while (WiFi.status() != WL_CONNECTED && millis() - startAttempt < 10000) {
    delay(500);
    tft.fillRect(150, 130, 40, 10, TFT_BLACK);
    tft.setCursor(150, 130);
    for (int i = 0; i < dots % 4; i++) tft.print(".");
    dots++;
  }
  
  tft.fillScreen(TFT_BLACK);
  drawScanlines();
  if (WiFi.status() == WL_CONNECTED) {
    wifiConnected = true;
    connectedSSID = String(ssid);

    tft.fillRect(40, 80, 240, 80, TFT_BLACK);
    tft.drawRect(40, 80, 240, 80, TFT_GREEN);
    tft.setTextColor(TFT_GREEN);
    tft.drawString("CONNECTED!", 160, 100);
    tft.drawString(WiFi.localIP().toString(), 160, 130);
    delay(2000);
  } else {
    tft.fillRect(40, 80, 240, 80, TFT_BLACK);
    tft.drawRect(40, 80, 240, 80, TFT_GREEN);
    tft.setTextColor(TFT_RED);
    tft.setTextSize(2);
    tft.drawString("FAILED!", 160, 110);
    tft.setTextSize(1);
    delay(2000);
  }
  
  wifiWaitingForPassword = false;
  drawWiFiScreen();
}

void wifiDisconnect() {
  WiFi.disconnect();
  wifiConnected = false;
  connectedSSID = "";
}

bool wifiIsConnected() {
  return (WiFi.status() == WL_CONNECTED);
}
