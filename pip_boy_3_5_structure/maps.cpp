#include "maps.h"
#include <PNGdec.h>

extern TFT_eSPI tft;

static PNG _png;
static File _pngFile;
static int16_t _drawSx = 0;
static int16_t _drawSy = 0;
static int16_t _clipLeft = 0;
static int16_t _clipRight = 389;
static int16_t _clipTop = 0;
static int16_t _clipBottom = 228;
static uint16_t _lineBuf[257];
static uint8_t  _idxBuf[257];

static TFT_eSprite* _spriteCanvas = nullptr;

// --- LUT для быстрой конверсии яркости в зелёный RGB565 ---
static uint16_t _greenLUT[256];
static bool     _lutReady = false;

static void initGreenLUT() {
    if (_lutReady) return;
    for (int i = 0; i < 256; i++) {
        uint8_t g = (i*45) / 60;
        uint8_t rb = g / 40;
        _greenLUT[i] = tft.color565(0, 0, g);   // R=0, G=i, B=0
    }
    _lutReady = true;
}

// --- Палитра PipBoy (4-bit) ---
#define PIP_BLACK   0
#define PIP_DARK2   4
#define PIP_WHITE   14
#define PIP_GREEN   15

static int PNGDrawCallback(PNGDRAW* pDraw) {
    if (pDraw->y % 16 == 0) delay(1);

    _png.getLineAsRGB565(pDraw, _lineBuf, PNG_RGB565_BIG_ENDIAN, 0xffffffff);

    int w = pDraw->iWidth;

    // Конверсия яркости 0..255
    uint8_t lumBuf[256];
    for (int i = 0; i < w && i < 256; i++) {
        uint16_t c = _lineBuf[i];
        uint8_t r = ((c >> 11) & 0x1F) << 3;
        uint8_t g = ((c >> 5) & 0x3F) << 2;
        uint8_t b = (c & 0x1F) << 3;
        lumBuf[i] = (r * 30 + g * 59 + b * 11) / 100;
    }

    if (_spriteCanvas) {
        // 4-bit спрайт: lum -> индекс палитры 0..13
        for (int i = 0; i < w && i < 256; i++) {
            _idxBuf[i] = (lumBuf[i] * 13) / 255;
        }
    } else {
        // Прямой TFT: lum -> чистый зелёный через LUT
        initGreenLUT();
        for (int i = 0; i < w && i < 256; i++) {
            _lineBuf[i] = _greenLUT[lumBuf[i]];
        }
    }

    int16_t drawY = _drawSy + pDraw->y;
    if (drawY < _clipTop || drawY >= _clipBottom) return 1;

    int16_t sx = _drawSx;
    int16_t drawW = w;
    int16_t srcOffset = 0;

    if (sx < _clipLeft) {
        srcOffset = _clipLeft - sx;
        drawW -= (_clipLeft - sx);
        sx = _clipLeft;
    }
    if (sx + drawW > _clipRight) {
        drawW = _clipRight - sx;
    }

    if (drawW > 0) {
        if (_spriteCanvas) {
            // Если pushImage с uint8_t не работает — рисуем по пикселю
            for (int i = 0; i < drawW; i++) {
                _spriteCanvas->drawPixel(sx + i, drawY, _idxBuf[srcOffset + i]);
            }
        } else {
            tft.pushImage(sx, drawY, drawW, 1, _lineBuf + srcOffset);
        }
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

MapsModule::MapsModule() : _progress(0), _loading(false), _useSprite(true), _zoom(16), _sprite(nullptr) {}

bool MapsModule::begin() {
    if (!SD.exists("/maps")) SD.mkdir("/maps");
    return true;
}

void MapsModule::disableSprite() {
    _useSprite = false;
    if (_sprite) {
        _sprite->deleteSprite();
        delete _sprite;
        _sprite = nullptr;
        _spriteCanvas = nullptr;
    }
    if (DEBUGFLAG) Serial.println("[MAPS] Sprite disabled, direct TFT output");
}

bool MapsModule::initSprite() {
    if (_sprite || !_useSprite) return true;

    int w = MAP_END_X - MAP_START_X;
    int h = MAP_END_Y - MAP_START_Y;

    _sprite = new TFT_eSprite(&tft);
    _sprite->setColorDepth(4);

    if (DEBUGFLAG) {
        Serial.printf("[MAPS] Need %d bytes for %dx%d 4-bit sprite\n", (w * h + 1) / 2, w, h);
        Serial.printf("[MAPS] Max alloc block: %d bytes\n", ESP.getMaxAllocHeap());
    }

    if (!_sprite->createSprite(w, h)) {
        Serial.printf("[MAPS] {initSprite} FAILED: need %d, max block %d\n", (w * h + 1) / 2, ESP.getMaxAllocHeap());
        delete _sprite;
        _sprite = nullptr;
        _useSprite = false;
        return false;
    }

    for (int i = 0; i < 14; i++) {
        uint8_t g = (i * 60) / 13;
        _sprite->setPaletteColor(i, (g << 5));
    }
    _sprite->setPaletteColor(PIP_WHITE, TFT_WHITE);
    _sprite->setPaletteColor(PIP_GREEN, TFT_GREEN);

    _spriteCanvas = _sprite;
    _clipLeft = 0; _clipRight = w; _clipTop = 0; _clipBottom = h;

    if (DEBUGFLAG) Serial.printf("[MAPS] {initSprite} OK: %dx%d, depth=4\n", w, h);
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

    int maxBlock = ESP.getMaxAllocHeap();
    if (maxBlock < 30000) {
        if (DEBUGFLAG) Serial.printf("[MAPS] {downloadTile} LOW RAM: %d bytes, skip\n", maxBlock);
        return false;
    }

    for (int attempt = 1; attempt <= 3; attempt++) {
        if (DEBUGFLAG) Serial.printf("[MAPS] {downloadTile} attempt %d/3: tile %d/%d z=%d\n", attempt, x, y, z);
        SD.remove(path);

        HTTPClient http;
        http.setTimeout(15000);
        http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);

        String url = "https://tile.openstreetmap.org/" + String(z) + "/" + String(x) + "/" + String(y) + ".png";
        http.begin(url);
        http.addHeader("User-Agent", "PipBoy-ESP32/1.0");
        http.addHeader("Accept", "image/png");

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

    if (cx == _centerX && cy == _centerY && _zoom == zoom && cachedCount(lat, lon, zoom) == 9) {
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
            delay(50);
        }
    }
    _loading = false;
    if (DEBUGFLAG) Serial.println("[MAPS] {ensureTiles} DONE");
    return true;
}

bool MapsModule::ensureExtendedTiles(float lat, float lon, uint8_t zoom, std::function<void(uint8_t)> progressCallback) {
    uint16_t cx, cy;
    latLonToTile(lat, lon, zoom, cx, cy);

    if (DEBUGFLAG) Serial.printf("[MAPS] {ensureExtendedTiles} START: center tile %d/%d, zoom=%d\n", cx, cy, zoom);

    _loading = true;
    _progress = 0;

    uint8_t total = 16;
    uint8_t done = 0;

    for (int dy = -2; dy <= 2; dy++) {
        for (int dx = -2; dx <= 2; dx++) {
            if (abs(dx) <= 1 && abs(dy) <= 1) continue;

            uint16_t tx = cx + dx;
            uint16_t ty = cy + dy;

            if (!SD.exists(tilePath(tx, ty, zoom))) {
                if (WiFi.status() == WL_CONNECTED) {
                    bool ok = downloadTile(tx, ty, zoom);
                    if (DEBUGFLAG) Serial.printf("[MAPS] {ext} [%d/%d]: %s\n", tx, ty, ok ? "OK" : "FAIL");
                } else {
                    if (DEBUGFLAG) Serial.printf("[MAPS] {ext} [%d/%d]: NO WIFI\n", tx, ty);
                    break;
                }
            } else {
                if (DEBUGFLAG) Serial.printf("[MAPS] {ext} [%d/%d]: CACHED\n", tx, ty);
            }
            done++;
            _progress = (done * 100) / total;
            if (progressCallback) progressCallback(_progress);
            delay(100);
        }
    }
    _loading = false;
    if (DEBUGFLAG) Serial.println("[MAPS] {ensureExtendedTiles} DONE");
    return true;
}

void MapsModule::drawPngTile(uint16_t x, uint16_t y, uint8_t z, int16_t sx, int16_t sy) {
    String p = tilePath(x, y, z);
    if (!SD.exists(p)) {
        if (DEBUGFLAG) Serial.printf("[MAPS] {drawPngTile} NOT FOUND: %s\n", p.c_str());
        drawPlaceholder(sx, sy);
        return;
    }

    if (sx + TILE_SIZE <= _clipLeft || sx >= _clipRight ||
        sy + TILE_SIZE <= _clipTop || sy >= _clipBottom) {
        if (DEBUGFLAG) Serial.printf("[MAPS] {drawPngTile} SKIP: %s (offscreen)\n", p.c_str());
        return;
    }

    if (DEBUGFLAG) Serial.printf("[MAPS] {drawPngTile} DRAW: %s at %d,%d\n", p.c_str(), sx, sy);

    _drawSx = sx;
    _drawSy = sy;

    int16_t rc = _png.open(p.c_str(), pngOpen, pngClose, pngRead, pngSeek, PNGDrawCallback);
    if (rc == PNG_SUCCESS) {
        if (!_spriteCanvas) tft.startWrite();
        _png.decode(NULL, 0);
        if (!_spriteCanvas) tft.endWrite();
        if (DEBUGFLAG) Serial.printf("[MAPS] {drawPngTile} DONE: %s\n", p.c_str());
    } else {
        if (DEBUGFLAG) Serial.printf("[MAPS] {drawPngTile} DECODE FAIL: %s rc=%d\n", p.c_str(), rc);
    }
    _png.close();
    delay(1);
}

void MapsModule::drawPlaceholder(int16_t sx, int16_t sy) {
    int16_t x0 = max(sx, (int16_t)_clipLeft);
    int16_t y0 = max(sy, (int16_t)_clipTop);
    int16_t x1 = min((int32_t)sx + TILE_SIZE, (int32_t)_clipRight);
    int16_t y1 = min((int32_t)sy + TILE_SIZE, (int32_t)_clipBottom);

    if (x1 <= x0 || y1 <= y0) return;

    int16_t w = x1 - x0;
    int16_t h = y1 - y0;

    if (_spriteCanvas) {
        _spriteCanvas->fillRect(x0, y0, w, h, PIP_BLACK);
        _spriteCanvas->drawRect(x0, y0, w, h, PIP_DARK2);
        _spriteCanvas->drawLine(x0, y0, x1, y1, PIP_DARK2);
        _spriteCanvas->drawLine(x1, y0, x0, y1, PIP_DARK2);
        if (sx >= _clipLeft && sx + TILE_SIZE <= _clipRight &&
            sy >= _clipTop && sy + TILE_SIZE <= _clipBottom) {
            _spriteCanvas->setTextDatum(MC_DATUM);
            _spriteCanvas->setTextColor(PIP_DARK2);
            _spriteCanvas->setTextSize(1);
            _spriteCanvas->drawString("NO DATA", sx + TILE_SIZE / 2, sy + TILE_SIZE / 2);
        }
    } else {
        tft.fillRect(x0, y0, w, h, TFT_BLACK);
        tft.drawRect(x0, y0, w, h, TFT_DARKGREEN);
        tft.drawLine(x0, y0, x1, y1, TFT_DARKGREEN);
        tft.drawLine(x1, y0, x0, y1, TFT_DARKGREEN);
        if (sx >= _clipLeft && sx + TILE_SIZE <= _clipRight &&
            sy >= _clipTop && sy + TILE_SIZE <= _clipBottom) {
            tft.setTextDatum(MC_DATUM);
            tft.setTextColor(TFT_DARKGREEN);
            tft.setTextSize(1);
            tft.drawString("NO DATA", sx + TILE_SIZE / 2, sy + TILE_SIZE / 2);
        }
    }
}

void MapsModule::drawMarker(int16_t mx, int16_t my) {
    if (_spriteCanvas) {
        _spriteCanvas->fillTriangle(mx, my - 8, mx - 6, my + 4, mx + 6, my + 4, PIP_GREEN);
        _spriteCanvas->drawTriangle(mx, my - 8, mx - 6, my + 4, mx + 6, my + 4, PIP_BLACK);
        _spriteCanvas->fillCircle(mx, my + 1, 2, PIP_WHITE);
    } else {
        tft.fillTriangle(mx, my - 8, mx - 6, my + 4, mx + 6, my + 4, TFT_GREEN);
        tft.drawTriangle(mx, my - 8, mx - 6, my + 4, mx + 6, my + 4, TFT_BLACK);
        tft.fillCircle(mx, my + 1, 2, TFT_WHITE);
    }
}

void MapsModule::drawMap(float lat, float lon) {
    redrawMap(lat, lon);
}

void MapsModule::redrawMap(float lat, float lon) {
    // Пробуем создать спрайт только если явно не отключён
    if (_useSprite && !_sprite) {
        initSprite();
    }

    uint16_t cx, cy;
    latLonToTile(lat, lon, _zoom, cx, cy);

    int16_t px, py;
    latLonToPixel(lat, lon, cx, cy, _zoom, px, py);

    int16_t areaW = MAP_END_X - MAP_START_X;
    int16_t areaH = MAP_END_Y - MAP_START_Y;

    // === Квантизация сдвига до 10px — меньше дрожания ===
    int16_t rawShiftX = areaW / 2 - px;
    int16_t rawShiftY = areaH / 2 - py;
    int16_t shiftX = (rawShiftX / 10) * 10;
    int16_t shiftY = (rawShiftY / 10) * 10;

    if (_useSprite && _sprite) {
        // ===== РЕЖИМ СПРАЙТА =====
        int16_t spriteW = _sprite->width();
        int16_t spriteH = _sprite->height();

        _clipLeft = 0; _clipRight = spriteW; _clipTop = 0; _clipBottom = spriteH;

        if (DEBUGFLAG) Serial.printf("[MAPS] {redrawMap} SPRITE mode, shift %d,%d\n", shiftX, shiftY);

        _sprite->fillSprite(PIP_BLACK);

        for (int dy = -1; dy <= 1; dy++) {
            for (int dx = -1; dx <= 1; dx++) {
                int16_t sx = shiftX + dx * TILE_SIZE;
                int16_t sy = shiftY + dy * TILE_SIZE;
                if (sx + TILE_SIZE <= 0 || sx >= spriteW || sy + TILE_SIZE <= 0 || sy >= spriteH) continue;
                drawPngTile(cx + dx, cy + dy, _zoom, sx, sy);
            }
        }

        drawMarker(spriteW / 2, spriteH / 2);
        _sprite->drawRect(0, 0, spriteW, spriteH, PIP_GREEN);

        _sprite->setTextDatum(BR_DATUM);
        _sprite->setTextColor(PIP_DARK2);
        _sprite->setTextSize(1);
        _sprite->drawString("OSM", spriteW - 4, spriteH - 4);

        _sprite->pushSprite(MAP_START_X, MAP_START_Y);

    } else {
        // ===== РЕЖИМ ПРЯМОГО ВЫВОДА (по умолчанию) =====
        _clipLeft = MAP_START_X;
        _clipRight = MAP_END_X;
        _clipTop = MAP_START_Y;
        _clipBottom = MAP_END_Y;
        _spriteCanvas = nullptr;

        if (DEBUGFLAG) Serial.printf("[MAPS] {redrawMap} TFT direct, shift %d,%d\n", shiftX, shiftY);

       // tft.setViewport(MAP_START_X, MAP_START_Y, areaW, areaH, true);
        //tft.fillRect(MAP_START_X, MAP_START_Y, areaW, areaH, TFT_BLACK);

        for (int dy = -1; dy <= 1; dy++) {
            for (int dx = -1; dx <= 1; dx++) {
                int16_t sx = MAP_START_X + shiftX + dx * TILE_SIZE;
                int16_t sy = MAP_START_Y + shiftY + dy * TILE_SIZE;
                if (sx + TILE_SIZE <= MAP_START_X || sx >= MAP_END_X ||
                    sy + TILE_SIZE <= MAP_START_Y || sy >= MAP_END_Y) continue;
                drawPngTile(cx + dx, cy + dy, _zoom, sx, sy);
            }
        }

        int16_t mx = MAP_START_X + areaW / 2;
        int16_t my = MAP_START_Y + areaH / 2;
        drawMarker(mx, my);

        tft.drawRect(MAP_START_X, MAP_START_Y, areaW, areaH, TFT_GREEN);

        //tft.setTextDatum(BR_DATUM);
        //tft.setTextColor(TFT_DARKGREEN);
        //tft.setTextSize(1);
        //tft.drawString("OSM", MAP_END_X - 4, MAP_END_Y - 4);

        tft.resetViewport();
    }
}

void MapsModule::setZoom(uint8_t z) {
    if (z >= 1 && z <= 19 && z != _zoom) {
        _zoom = z;
        _centerX = 0xFFFF;
        _centerY = 0xFFFF;
        if (DEBUGFLAG) Serial.printf("[MAPS] setZoom -> %d\n", _zoom);
    }
}

uint8_t MapsModule::getZoom() const {
    return _zoom;
}

void MapsModule::toggleZoom() {
    setZoom(_zoom == MAP_ZOOM_IN ? MAP_ZOOM_OUT : MAP_ZOOM_IN);
}
