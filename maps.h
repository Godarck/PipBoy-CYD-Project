#ifndef MAPS_H
#define MAPS_H

#include <Arduino.h>
#include <functional>
#include <TFT_eSPI.h>
#include <SD.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include "config.h"

class MapsModule {
public:
    MapsModule();
    bool begin();
    bool initSprite();                         // <-- создать спрайт (вызвать 1 раз)
    bool ensureTiles(float lat, float lon, uint8_t zoom = MAP_ZOOM_IN, std::function<void(uint8_t)> progressCallback = nullptr);
    void drawMap(float lat, float lon);
    void redrawMap(float lat, float lon);      // <-- обновить только карту без мерцания
    uint8_t cachedCount(float lat, float lon, uint8_t zoom = MAP_ZOOM_IN);
    uint8_t getProgress() const { return _progress; }
    bool isLoading() const { return _loading; }
    void setZoom(uint8_t z);
    uint8_t getZoom() const;
    void toggleZoom();
    void disableSprite();
    bool ensureExtendedTiles(float lat, float lon, uint8_t zoom = MAP_ZOOM_IN, std::function<void(uint8_t)> progressCallback = nullptr);
    
private:
    uint8_t _progress;
    bool _loading;
    bool _useSprite;
    uint8_t _zoom;
    static const uint16_t TILE_SIZE = 256;
    static const uint8_t GRID = 3;
    uint16_t _centerX = 0xFFFF;
    uint16_t _centerY = 0xFFFF;
    TFT_eSprite* _sprite;                      // <-- off-screen buffer

    void latLonToTile(float lat, float lon, uint8_t zoom, uint16_t& x, uint16_t& y);
    void latLonToPixel(float lat, float lon, uint16_t tileX, uint16_t tileY, uint8_t zoom, int16_t& px, int16_t& py);
    String tilePath(uint16_t x, uint16_t y, uint8_t z);
    bool ensureDir(uint16_t x, uint8_t z);
    bool downloadTile(uint16_t x, uint16_t y, uint8_t z);
    void drawPngTile(uint16_t x, uint16_t y, uint8_t z, int16_t sx, int16_t sy);
    void drawPlaceholder(int16_t sx, int16_t sy);
    void drawMarker(int16_t mx, int16_t my);
};

#endif
