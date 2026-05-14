#include "maps.h"
#include <PNGdec.h>

extern TFT_eSPI tft;

static PNG _png;
static File _pngFile;
static int16_t _drawSx = 5;
static int16_t _drawSy = 0;
static int16_t _clipTop = 40;
static int16_t _clipBottom = 320;
static uint16_t _lineBuf[257];

static int PNGDrawCallback(PNGDRAW* pDraw) {
    if (pDraw->y % 16 == 0) delay(1);

    _png.getLineAsRGB565(pDraw, _lineBuf, PNG_RGB565_BIG_ENDIAN, 0xffffffff);

    int w = pDraw->iWidth;

#if CHANGE_PIP_COLOR
    for (int i = 0; i < w && i < 256; i++) {
        uint16_t c = _lineBuf[i];
        uint8_t r = ((c >> 11) & 0x1F) << 3;
        uint8_t g = ((c >> 5) & 0x3F) << 2;
        uint8_t b = (c & 0x1F) << 3;

        uint8_t lum = (r * 30 + g * 59 + b * 11) / 100;
        // Монохромный зелёный: яркость = насыщенность зелёного
        uint8_t gg = min(255, (lum * 140) / 100);
        uint8_t dg = lum >> 3; // тёмный оттенок для теней
        r = dg;
        g = dg;
        b = gg;

        _lineBuf[i] = ((r >> 3) << 11) | ((g >> 2) << 5) | (b >> 3);
    }
#endif




    int16_t drawY = _drawSy + pDraw->y;
    if (drawY < 0 || drawY >= TFT_HEIGHT_SCREEN - TAB_H - 5) return 1;
    if (drawY < _clipTop || drawY >= _clipBottom) return 1;

    int16_t sx = _drawSx;
    int16_t drawW = w;
    int16_t srcOffset = 0;
    if (sx < _drawSx) { srcOffset = -sx; drawW += sx; sx = _drawSx; }
    if (sx + drawW > TFT_WIDTH_SCREEN - 10) drawW = TFT_WIDTH_SCREEN - 5 - sx;
    if (drawW > 0) {
        tft.pushImage(sx, drawY, drawW, 1, _lineBuf + srcOffset);
        tft.dmaWait();
    }
    return 1;
}

static void* pngOpen(const char* filename, int32_t* size) {
    _pngFile = SD.open(filename, "r");
    if (!_pngFile) {
        if (DEBUGFLAG) Serial.printf("[MAPS] {pngOpen} FAIL: %s\n", filename);
        return NULL;
    }
    *size = _pngFile.size();
    return &_pngFile;
}

static void pngClose(void* handle) {
    if (_pngFile) _pngFile.close();
}

static int32_t pngRead(PNGFILE* page, uint8_t* buffer, int32_t length) {
    if (!_pngFile) return 0;
    page = page;
    return _pngFile.read(buffer, length);
}

static int32_t pngSeek(PNGFILE* page, int32_t position) {
    if (!_pngFile) return 0;
    page = page;
    _pngFile.seek(position);
    return position;
}

// ==================== MapsModule ====================

MapsModule::MapsModule() : _progress(0), _loading(false), _zoom(16) {}

bool MapsModule::begin() {
    if (!SD.exists("/maps")) SD.mkdir("/maps");
    return true;
}

void MapsModule::latLonToTile(float lat, float lon, uint8_t zoom, uint16_t& x, uint16_t& y) {
    uint32_t n = 1UL << zoom;
    x = (uint16_t)((lon + 180.0f) / 360.0f * n);
    float latRad = lat * PI / 180.0f;
    y = (uint16_t)((1.0f - logf(tanf(latRad) + 1.0f / cosf(latRad)) / PI) / 2.0f * n);
}

void MapsModule::latLonToPixel(float lat, float lon, uint16_t tileX, uint16_t tileY, uint8_t zoom, int16_t& px, int16_t& py) {
    uint32_t n = 1UL << zoom;
    float x = (lon + 180.0f) / 360.0f * n;
    float latRad = lat * PI / 180.0f;
    float y = (1.0f - logf(tanf(latRad) + 1.0f / cosf(latRad)) / PI) / 2.0f * n;
    px = (int16_t)((x - tileX) * TILE_SIZE);
    py = (int16_t)((y - tileY) * TILE_SIZE);
}

String MapsModule::tilePath(uint16_t x, uint16_t y, uint8_t z) {
    return "/maps/" + String(z) + "/" + String(x) + "/" + String(y) + ".png";
}

bool MapsModule::ensureDir(uint16_t x, uint8_t z) {
    String zd = "/maps/" + String(z);
    String xd = zd + "/" + String(x);
    if (!SD.exists(zd)) SD.mkdir(zd);
    if (!SD.exists(xd)) SD.mkdir(xd);
    return true;
}

bool MapsModule::downloadTile(uint16_t x, uint16_t y, uint8_t z) {
    ensureDir(x, z);
    String path = tilePath(x, y, z);
    if (SD.exists(path)) return true;

    for (int attempt = 1; attempt <= 3; attempt++) {
        if (DEBUGFLAG) Serial.printf("[MAPS] {downloadTile} attempt %d/3: tile %d/%d z=%d\n", attempt, x, y, z);
        SD.remove(path);

        HTTPClient http;
        http.setTimeout(1500);
        String url = "https://tile.openstreetmap.org/" + String(z) + "/" + String(x) + "/" + String(y) + ".png";
        http.begin(url);
        http.addHeader("User-Agent", "PipBoy-ESP32/1.0");
        int code = http.GET();

        if (code != 200) {
            if (DEBUGFLAG) Serial.printf("[MAPS] {downloadTile} HTTP error: %d\n", code);
            http.end();
            delay(500);
            continue;
        }

        File f = SD.open(path, FILE_WRITE);
        if (!f) {
            if (DEBUGFLAG) Serial.println("[MAPS] {downloadTile} FAIL: SD open");
            http.end();
            return false;
        }

        int written = http.writeToStream(&f);
        f.close();
        http.end();

        if (written > 0) {
            if (DEBUGFLAG) Serial.printf("[MAPS] {downloadTile} OK: %d bytes\n", written);
            return true;
        }

        SD.remove(path);
        delay(500);
    }
    if (DEBUGFLAG) Serial.printf("[MAPS] {downloadTile} ALL ATTEMPTS FAILED: tile %d/%d\n", x, y);
    return false;
}

uint8_t MapsModule::cachedCount(float lat, float lon, uint8_t zoom) {
    uint16_t cx, cy;
    latLonToTile(lat, lon, zoom, cx, cy);
    uint8_t n = 0;
    for (int dy = -1; dy <= 1; dy++)
        for (int dx = -1; dx <= 1; dx++)
            if (SD.exists(tilePath(cx + dx, cy + dy, zoom))) n++;
    return n;
}

bool MapsModule::ensureTiles(float lat, float lon, uint8_t zoom, std::function<void(uint8_t)> progressCallback) {
    uint16_t cx, cy;
    latLonToTile(lat, lon, zoom, cx, cy);

    if (cx == _centerX && cy == _centerY && cachedCount(lat, lon, zoom) == 9) {
        if (DEBUGFLAG) Serial.println("[MAPS] {ensureTiles} ALL CACHED, skip");
        return true;
    }

    _centerX = cx;
    _centerY = cy;
    _zoom = zoom;
    _loading = true;
    _progress = 0;

    uint8_t total = GRID * GRID;
    uint8_t done = 0;

    if (DEBUGFLAG) Serial.printf("[MAPS] {ensureTiles} START: center tile %d/%d, zoom=%d\n", cx, cy, zoom);

    for (int dy = -1; dy <= 1; dy++) {
        for (int dx = -1; dx <= 1; dx++) {
            uint16_t tx = cx + dx;
            uint16_t ty = cy + dy;
            if (!SD.exists(tilePath(tx, ty, zoom))) {
                if (WiFi.status() == WL_CONNECTED) {
                    bool ok = downloadTile(tx, ty, zoom);
                    if (DEBUGFLAG) Serial.printf("[MAPS] {ensureTiles} tile %d/%d [%d/%d]: %s\n", done+1, total, tx, ty, ok ? "OK" : "FAIL");
                } else {
                    if (DEBUGFLAG) Serial.printf("[MAPS] {ensureTiles} tile %d/%d [%d/%d]: NO WIFI\n", done+1, total, tx, ty);
                }
            } else {
                if (DEBUGFLAG) Serial.printf("[MAPS] {ensureTiles} tile %d/%d [%d/%d]: CACHED\n", done+1, total, tx, ty);
            }
            done++;
            _progress = (done * 100) / total;
            if (progressCallback) progressCallback(_progress);
            delay(500);
        }
    }
    _loading = false;
    if (DEBUGFLAG) Serial.println("[MAPS] {ensureTiles} DONE");
    return true;
}

void MapsModule::drawPngTile(uint16_t x, uint16_t y, uint8_t z, int16_t sx, int16_t sy) {
    String p = tilePath(x, y, z);
    if (!SD.exists(p)) {
        if (DEBUGFLAG) Serial.printf("[MAPS] {drawPngTile} NOT FOUND: %s\n", p.c_str());
        drawPlaceholder(sx, sy);
        return;
    }

    if (sx + TILE_SIZE <= 0 || sx >= TFT_WIDTH_SCREEN || 
        sy + TILE_SIZE <= 0 || sy >= TFT_HEIGHT_SCREEN) {
        if (DEBUGFLAG) Serial.printf("[MAPS] {drawPngTile} SKIP: %s (offscreen)\n", p.c_str());
        return;
    }

    if (DEBUGFLAG) Serial.printf("[MAPS] {drawPngTile} DRAW: %s at %d,%d\n", p.c_str(), sx, sy);

    _drawSx = sx;
    _drawSy = sy;

    int16_t rc = _png.open(p.c_str(), pngOpen, pngClose, pngRead, pngSeek, PNGDrawCallback);
    if (rc == PNG_SUCCESS) {
        tft.startWrite();
        _png.decode(NULL, 0);
        tft.endWrite();
        if (DEBUGFLAG) Serial.printf("[MAPS] {drawPngTile} DONE: %s\n", p.c_str());
    } else {
        if (DEBUGFLAG) Serial.printf("[MAPS] {drawPngTile} DECODE FAIL: %s rc=%d\n", p.c_str(), rc);
    }
    _png.close();
    delay(1);
}

void MapsModule::drawPlaceholder(int16_t sx, int16_t sy) {
    tft.fillRect(sx, sy, TILE_SIZE, TILE_SIZE, TFT_BLACK);
    tft.drawRect(sx, sy, TILE_SIZE, TILE_SIZE, TFT_DARKGREEN);
    tft.drawLine(sx, sy, sx + TILE_SIZE, sy + TILE_SIZE, TFT_DARKGREEN);
    tft.drawLine(sx + TILE_SIZE, sy, sx, sy + TILE_SIZE, TFT_DARKGREEN);
    tft.setTextDatum(MC_DATUM);
    tft.setTextColor(TFT_DARKGREEN);
    tft.setTextSize(1);
    tft.drawString("NO DATA", sx + TILE_SIZE / 2, sy + TILE_SIZE / 2);
}

void MapsModule::drawMarker(int16_t mx, int16_t my) {
    tft.fillTriangle(mx, my - 8, mx - 6, my + 4, mx + 6, my + 4, TFT_GREEN);
    tft.drawTriangle(mx, my - 8, mx - 6, my + 4, mx + 6, my + 4, TFT_BLACK);
    tft.fillCircle(mx, my + 1, 2, TFT_WHITE);
}

void MapsModule::drawMap(float lat, float lon, uint16_t areaX, uint16_t areaY) {
    uint16_t cx, cy;
    latLonToTile(lat, lon, _zoom, cx, cy);

    int16_t px, py;
    latLonToPixel(lat, lon, cx, cy, _zoom, px, py);

    uint16_t areaW = TFT_WIDTH_SCREEN - areaX * 2;
    uint16_t areaH = SCREEN_BOTTOM_Y - areaY - 5;

    _clipTop = areaY;
    _clipBottom = SCREEN_BOTTOM_Y - 5;

    int16_t shiftX = areaX + areaW / 2 - px;
    int16_t shiftY = areaY + areaH / 2 - py;

    if (DEBUGFLAG) Serial.printf("[MAPS] {drawMap} center tile %d/%d, shift %d,%d\n", cx, cy, shiftX, shiftY);

    for (int dy = -1; dy <= 1; dy++) {
        for (int dx = -1; dx <= 1; dx++) {
            int16_t sx = shiftX + dx * TILE_SIZE;
            int16_t sy = shiftY + dy * TILE_SIZE;

            if (sx + TILE_SIZE <= areaX || sx >= TFT_WIDTH_SCREEN - areaX*2 ||
                sy + TILE_SIZE <= areaY || sy >= TFT_HEIGHT_SCREEN - SCREEN_BOTTOM_Y) {
                continue;
            }

            drawPngTile(cx + dx, cy + dy, _zoom, sx, sy);
        }
    }

    int16_t mx = areaX + areaW / 2;
    int16_t my = areaY + areaH / 2;
    drawMarker(mx, my);

    tft.drawRect(areaX, areaY, areaW, areaH, TFT_GREEN);
}
