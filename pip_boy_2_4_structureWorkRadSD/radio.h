#ifndef RADIO_H
#define RADIO_H

#include <Arduino.h>
#include <vector>
#include <AudioOutputI2SNoDAC.h>
#include <AudioGeneratorMP3.h>
#include <AudioFileSourceBuffer.h>
#include <AudioFileSourceICYStream.h>
#include <AudioFileSourceSD.h>

/* ===== Extern-переменные из основного скетча ===== */
extern bool  sdCardInitialized;
extern int   radioPlaySource;      // 0=SD, 1=WiFi, 2=Ext
extern String radioSDFolder;
extern std::vector<String> sdPlaylist;
extern int   currentSDTrack;
extern bool  sdPlaylistLoaded;

/* ===== Метаданные ===== */
extern char currentTitle[256];
extern char currentArtist[256];
extern char currentSongInfo[512];

struct RadioStation {
    const char* name;
    const char* url;
};

extern const RadioStation radioStations[];
extern const int radioStationCount;

/* ===== API ===== */
void radioInit();
void radioStartTask();    // запускает таск на Core 0
void radioStopTask();

// Эти функции потокобезопасны — посылают команду в очередь
void radioPlayStation(int index);
void radioPlaySDTrack(int index);
void radioPlaySDFolder(const String& folder);
void radioPlay();         // play/resume текущего source
void radioPause();        // stop
void radioStop();
void radioSetVolume(uint8_t vol);   // 0..100

// Навигация (тоже через очередь)
bool radioNextStation();
bool radioPrevStation();
bool radioNextTrack();
bool radioPrevTrack();

// Папки SD (не трогают аудио, можно звать из UI напрямую)
void radioSetSDFolder(const String& folder);
int  radioScanSDFolder();
int  radioGetSDTrackCount();
const char* radioGetSDTrackName();

// Геттеры
uint8_t radioGetVolume();
bool    radioIsPlaying();
int     radioGetStationIndex();
const char* radioGetStationName();
const char* radioGetCurrentTitle();
const char* radioGetCurrentArtist();
const char* radioGetCurrentSong();

#endif
