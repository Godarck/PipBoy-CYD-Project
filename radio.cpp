#include "radio.h"
#include <AudioFileSourceICYStream.h>
#include <AudioFileSourceBuffer.h>
#include <AudioFileSourceSD.h>
#include <AudioFileSourceID3.h>
#include <AudioGeneratorMP3.h>
#include <AudioOutputI2SNoDAC.h>
#include <Arduino.h>
#include <WiFi.h>
#include <SD.h>

extern bool  sdCardInitialized;
extern int   radioPlaySource;
extern String radioSDFolder;
extern std::vector<String> sdPlaylist;
extern int   currentSDTrack;
extern bool  sdPlaylistLoaded;
//extern void UpdateMetaData();

/* ===== Аудио-объекты (только radioTask их трогает!) ===== */
static AudioOutputI2SNoDAC   *out  = nullptr;
static AudioGenerator        *gen  = nullptr;
static AudioFileSource       *src  = nullptr;
static AudioFileSourceBuffer *buff = nullptr;
static AudioFileSourceID3 *id3 = nullptr;

/* ===== Состояние ===== */
static int        curStationIdx = 0;
static int        curTrackIdx   = 0;
static uint8_t    curVolume     = 50;
static volatile bool isPlayingFlag = false;
static volatile bool softStopRequested = false;

/* ===== FreeRTOS: очередь команд + semaphore для экстренного стопа ===== */
static QueueHandle_t cmdQueue      = NULL;
static SemaphoreHandle_t stopSem   = NULL;
static TaskHandle_t taskHandle     = NULL;

/* ===== Метаданные ===== */
char currentTitle[256]    = {0};
char currentArtist[256]   = {0};
char currentSongInfo[512] = {0};

const RadioStation radioStations[] = {
    {"Groove Salad",    "http://ice1.somafm.com/groovesalad-128-mp3"},
    {"Radio Paradise",  "http://stream.radioparadise.com/mp3-128"},
    {"Nashe Radio",     "http://nashe1.hostingradio.ru:80/nashe-128.mp3"},
    {"Nashe2.0",        "http://nashe2.hostingradio.ru:80/nashe20-128.mp3"},
    {"Radio Zenit",     "http://radio.zenit.ru:8000/zenit128"},
    {"Radio Punk",      "http://stream.punkradio.ru:8000/punk128"},
    {"Radio Metal",     "http://stream.metallradio.ru:8000/metal128"},
    {"Evropa +",        "http://ep128.hostingradio.ru:8059/evropaplus128.mp3"},
    {"Russkoe Radio",   "http://rusradio.hostingradio.ru:80/rusradio-128.mp3"},
    {"HIT FM",          "http://hitfm.hostingradio.ru:80/hitfm-128.mp3"},
    {"Radio Dacha",     "http://radiodacha.hostingradio.ru:8002/radiodacha128.mp3"},
    {"Love Radio",      "http://loveradio.hostingradio.ru:80/loveradio-128.mp3"}
};
const int radioStationCount = sizeof(radioStations)/sizeof(radioStations[0]);

/* ===== ICY callback ===== */
static void mdCallback(void *cbData, const char *type, bool isUnicode, const char *string)
{
    (void)cbData; (void)isUnicode;
    if (strcmp(type, "StreamTitle") == 0) {
        const char* sep = strstr(string, " - ");
        if (sep) {
            int len = sep - string; if (len > 255) len = 255;
            memcpy(currentArtist, string, len); currentArtist[len] = '\0';
            strncpy(currentTitle, sep + 3, 255);
        } else {
            strncpy(currentTitle, string, 255); currentArtist[0] = '\0';
        }
        currentTitle[255] = '\0'; currentArtist[255] = '\0';
        snprintf(currentSongInfo, 512, "%s - %s",
                 currentArtist[0] ? currentArtist : "Unknown Artist",
                 currentTitle[0]  ? currentTitle  : "Unknown Track");
    }
}

/* ===== Внутренние (вызываются только из radioTask) ===== */
static void cleanup()
{
    if (gen)  { gen->stop();  delete gen;  gen = nullptr; }
    if (buff) { delete buff; buff = nullptr; }
    if (src)  { src->close(); delete src;  src = nullptr; }
    isPlayingFlag = false;
    if (out) out->SetGain(0);
}

static void doStop()
{
    cleanup();
    isPlayingFlag = false;
   // currentTitle[0] = '\0'; currentArtist[0] = '\0';
    snprintf(currentSongInfo, 512, "Stopped");
    Serial.println("[RADIO] Stopped");
}

static void loadPlaylist(const String& folder)
{
    sdPlaylist.clear(); sdPlaylistLoaded = false;
    if (!sdCardInitialized) return;

    File dir = SD.open(folder.c_str());
    if (!dir || !dir.isDirectory()) { if(dir) dir.close(); return; }

    int n = 0;
    File f = dir.openNextFile();
    while (f) {
        if (!f.isDirectory()) {
            String name = f.name();
            if ((name.endsWith(".mp3") || name.endsWith(".MP3")) && (f.size() > 8096)) {
                sdPlaylist.push_back(name);
                Serial.printf("[RADIO]  [%d] %s - size:%d\n", n++, name.c_str(), f.size() );
            }
        }
        f.close(); f = dir.openNextFile();
    }
    dir.close();
    sdPlaylistLoaded = (n > 0);
    Serial.printf("[RADIO] Playlist: %d tracks\n", n);
}

static void startWiFi(int index)
{
    if (index < 0 || index >= radioStationCount) return;
    if (WiFi.status() != WL_CONNECTED) { Serial.println("[RADIO] ERR: no WiFi"); return; }
    cleanup();
    curStationIdx = index;
    radioPlaySource = 1;

    src = new AudioFileSourceICYStream(radioStations[index].url);
    ((AudioFileSourceICYStream*)src)->RegisterMetadataCB(mdCallback, NULL);
    buff = new AudioFileSourceBuffer(src, 32768);
    gen  = new AudioGeneratorMP3();

    if (gen->begin(buff, out)) {
        out->SetGain(curVolume / 100.0);
        isPlayingFlag = true;
        snprintf(currentSongInfo, 512, "%s", radioStations[index].name);
        //UpdateMetaData();
        Serial.printf("[RADIO] WiFi OK: %s\n", radioStations[index].name);
    } else {
        Serial.println("[RADIO] WiFi begin FAILED");
        cleanup();
    }
}

static void startSD(int index)
{
    if (!sdCardInitialized || sdPlaylist.empty()) { Serial.println("[RADIO] ERR: SD not ready"); return; }
    if (index < 0 || index >= (int)sdPlaylist.size()) return;

    cleanup();
    curTrackIdx = index;
    currentSDTrack = index;
    radioPlaySource = 0;

    String path = radioSDFolder + "/" + sdPlaylist[index];
    Serial.printf("[RADIO] SD path: %s\n", path.c_str());

    src = new AudioFileSourceSD(path.c_str());
    
    id3 = new AudioFileSourceID3(src);

    ((AudioFileSourceID3*)src)->RegisterMetadataCB(mdCallback, NULL);
    //id3->RegisterMetadataCB(AudioStatus::metadataCBFn fn, void *data)
    buff = new AudioFileSourceBuffer(src, 4096);   // <-- БУФЕР! loop() не ждёт SPI
    gen = new AudioGeneratorMP3();

    if (gen->begin(buff, out)) {
        out->SetGain(curVolume / 100.0);
        isPlayingFlag = true;
        snprintf(currentSongInfo, 512, "%s", sdPlaylist[index].c_str());
        //UpdateMetaData();
        Serial.println("[RADIO] SD OK");
    } else {
        Serial.println("[RADIO] SD begin FAILED");
        cleanup();
    }
}

/* ===== Команды ===== */
typedef enum { CMD_PLAY_WIFI, CMD_PLAY_SD, CMD_NEXT, CMD_PREV, CMD_SET_VOL } CmdType;
struct Cmd { CmdType type; int value; };

static void processCmd(const Cmd& c)
{
    switch (c.type) {
        case CMD_PLAY_WIFI: startWiFi(c.value); break;
        case CMD_PLAY_SD:   startSD(c.value); break;
        case CMD_NEXT:
            if (radioPlaySource == 1) {
                int i = curStationIdx + 1; if (i >= radioStationCount) i = 0; startWiFi(i);
            } else if (radioPlaySource == 0 && !sdPlaylist.empty()) {
                int i = curTrackIdx + 1; if (i >= (int)sdPlaylist.size()) i = 0; startSD(i);
            }
            break;
        case CMD_PREV:
            if (radioPlaySource == 1) {
                int i = curStationIdx - 1; if (i < 0) i = radioStationCount - 1; startWiFi(i);
            } else if (radioPlaySource == 0 && !sdPlaylist.empty()) {
                int i = curTrackIdx - 1; if (i < 0) i = sdPlaylist.size() - 1; startSD(i);
            }
            break;
        case CMD_SET_VOL:
            curVolume = constrain(c.value, 0, 100);
            if (out) out->SetGain(curVolume / 100.0);
            Serial.printf("[RADIO] Vol=%d\n", curVolume);
            break;
    }
}

/* ===== Публичный API ===== */

void radioInit()
{
    Serial.println("[RADIO] Init");
    if (!out) {
        out = new AudioOutputI2SNoDAC(26);  // ваш вариант с пином в конструкторе
        out->SetGain(0);
    }
    if (!cmdQueue) cmdQueue = xQueueCreate(16, sizeof(Cmd));
    if (!stopSem)  stopSem  = xSemaphoreCreateBinary();  // пуст изначально
}

void radioStartTask()
{
    if (taskHandle) return;
    xTaskCreatePinnedToCore([](void*) {
        Serial.printf("[RADIO] Task on Core %d\n", xPortGetCoreID());
        Cmd cmd;
        int wifiFail = 0;

        while (true) {
            // 1. === ЭКСТРЕННЫЙ СТОП (semaphore) ===
            // Проверяем с таймаутом 0 — не блокируемся!
            if (stopSem && xSemaphoreTake(stopSem, 0) == pdTRUE) {
                doStop();
                softStopRequested = false;
                wifiFail = 0;
                // после стопа пропускаем остальное в этой итерации
            }
            if (softStopRequested)
            {
                vTaskDelay(pdMS_TO_TICKS(5));
                continue;
            }
            
            //else {
                // 2. === Команды из очереди (play, next, vol) ===
                while (xQueueReceive(cmdQueue, &cmd, 0) == pdTRUE) {
                    processCmd(cmd);
                }

                // 3. === Один аудио-фрейм ===
                if (gen && gen->isRunning()) {
                    //bool ok = gen->loop();
                    if (!gen->loop()) {
                        if (radioPlaySource == 1) {
                            wifiFail++;
                            if (wifiFail > 50) { 
                                if (!softStopRequested) 
                                {
                                    Serial.println("[RADIO] WiFi retry");
                                    startWiFi(curStationIdx);
                                }
                                wifiFail = 0; 
                            }
                        } else if (radioPlaySource == 0) {
                            if (!softStopRequested) 
                            {
                                // если нужно автовоспроизведение по порядку
                                //int i = curTrackIdx + 1;
                                //if (i >= (int)sdPlaylist.size()) i = 0;
                                //startSD(i);
                                //UpdateMetaData();
                            }
                        }
                    } else {
                        if (wifiFail) wifiFail = 0;
                    }
                }
            //}

            // 4. === Отдаём CPU UI на Core 1 ===
            vTaskDelay(pdMS_TO_TICKS(2));
        }
    }, "RadioTask", 8192, NULL, 1, &taskHandle, 0);
}

void radioStopTask()
{
    if (taskHandle) { vTaskDelete(taskHandle); taskHandle = NULL; }
    doStop();
}

// --- Отправка команд ---
static inline void sendCmd(CmdType t, int v = 0) {
    if (!cmdQueue) return;
    Cmd c = {t, v};
    if (xQueueSend(cmdQueue, &c, pdMS_TO_TICKS(100)) != pdPASS)
        Serial.println("[RADIO] ERR: queue full");
}






// === radioStop / radioPause через semaphore ===
// Это самый быстрый способ — таск увидит его в течение 2 мс
void radioStop() {
    if (isPlayingFlag)
    {
        Serial.println("[RADIO] Stop (sem)");
        //out->SetGain(0);
        isPlayingFlag = false;
        softStopRequested = true;
        if (src && src->isOpen()) 
        {
            src->close();   
            
        }
        if (out)
            {
                out->flush();
                out->stop();
            }
        
        //if (gen->isRunning()) gen->stop();
        xSemaphoreGive(stopSem);
    }
}

void radioSetVolume(uint8_t vol) {

    curVolume = constrain(vol, 0, 100);
    out->SetGain(vol / 100.0); 
    curVolume = vol;
    Serial.printf("[RADIO] Volume=%d\n", curVolume);
} //sendCmd(CMD_SET_VOL, vol); }







void radioPause() { radioStop(); }   // Pause = Stop в вашей логике

// === Остальное через очередь ===
void radioPlayStation(int index) { sendCmd(CMD_PLAY_WIFI, index); }
void radioPlaySDTrack(int index) { sendCmd(CMD_PLAY_SD, index); }
void radioPlaySDFolder(const String& folder) {
    radioSDFolder = folder;
    loadPlaylist(folder);
    if (sdPlaylistLoaded) sendCmd(CMD_PLAY_SD, 0);
}

void radioPlay() {
    if (isPlayingFlag) return;
    if (radioPlaySource == 1) sendCmd(CMD_PLAY_WIFI, curStationIdx);
    else if (radioPlaySource == 0) sendCmd(CMD_PLAY_SD, curTrackIdx);
}


bool radioNextTrack() {
    if (sdPlaylist.empty()) 
    {
        if (radioScanSDFolder() < 0)
            return false;
        
    }

    
    // Если SD не играет — переключаем трек
    if (!isPlayingFlag && radioPlaySource == 0) {
        //sendCmd(CMD_PLAY_SD, curTrackIdx);
        curTrackIdx++;
         if (curTrackIdx >= (int)sdPlaylist.size()) curTrackIdx = 0;
         return true;
    }
    return false;//sdPlaylist[curTrackIdx].c_str();
}

bool radioPrevTrack() {
    if (sdPlaylist.empty()) 
    {
        if (radioScanSDFolder() < 0)
            return false;
        
    }

    if (!isPlayingFlag && radioPlaySource == 0) {
   //     sendCmd(CMD_PLAY_SD, curTrackIdx);
       curTrackIdx--;
    if (curTrackIdx < 0) curTrackIdx = sdPlaylist.size() - 1;
    return true;
    }
    return false;//sdPlaylist[curTrackIdx].c_str();
}

bool radioNextStation() {

    // Если WiFi не играет — переключаем станцию
    if (!isPlayingFlag && radioPlaySource == 1) {
   //     sendCmd(CMD_PLAY_WIFI, curStationIdx);
       curStationIdx++;
            if (curStationIdx >= radioStationCount) curStationIdx = 0;
    return true;
    }
    return false;
}

bool radioPrevStation() {
    
     if (!isPlayingFlag && radioPlaySource == 1) {
   //     sendCmd(CMD_PLAY_WIFI, curStationIdx);
   curStationIdx--;
    if (curStationIdx < 0) curStationIdx = radioStationCount - 1;
    return true;
    }
    return false;
}


// --- SD-функции (UI) ---
void radioSetSDFolder(const String& folder) {
    radioSDFolder = folder;
    radioScanSDFolder();
}

int radioScanSDFolder() {
    loadPlaylist(radioSDFolder);
    return sdPlaylist.size();
}
int radioGetSDTrackCount() { return sdPlaylist.size(); }
const char* radioGetSDTrackName() {
    return (curTrackIdx >= 0 && curTrackIdx < (int)sdPlaylist.size()) ? sdPlaylist[curTrackIdx].c_str() : "";
}

// --- Геттеры ---
uint8_t radioGetVolume() { return curVolume; }
bool    radioIsPlaying() { return isPlayingFlag; }
int     radioGetStationIndex() { return curStationIdx; }
const char* radioGetStationName() {
    return (curStationIdx >= 0 && curStationIdx < radioStationCount) ? radioStations[curStationIdx].name : "Unknown";
}
const char* radioGetCurrentTitle()  { 
    return currentTitle; }
const char* radioGetCurrentArtist() { return currentArtist; }
const char* radioGetCurrentSong()   { return currentSongInfo; }
