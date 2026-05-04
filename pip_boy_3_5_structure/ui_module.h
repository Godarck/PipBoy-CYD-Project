#ifndef UI_MODULE_H
#define UI_MODULE_H

#include <Arduino.h>

// Глобальные переменные клавиатуры
extern bool keyboardActive;
extern char inputBuffer[32];
extern bool KeyboardEnter;
extern bool shiftActive;


// Состояние клавиатуры

extern int inputLength;
extern int cursorPos;
extern bool pcursor;

// Типы клавиш
#define KEY_CHAR 0
#define KEY_BACKSPACE 1
#define KEY_ENTER 2
#define KEY_SHIFT 3
#define KEY_SPACE 4
#define KEY_ESC 6

// Структура клавиш
typedef struct {
  char label[6];
  int16_t x, y;
  int16_t w, h;
  uint8_t type;
} Key;

extern Key keys[55];
extern int keyCount;

extern const char* layoutUpper[5][10];
extern const char* layoutLower[5][10];

// Инициализация
void uiInit();

// Клавиатура
void uiDrawKeyboard();
bool uiHandleKeyboardTouch(int x, int y);  // true если касание обработано
void uiShowKeyboard(const char* placeholder);
void uiHideKeyboard();
const char* uiGetKeyboardInput();
void uiClearKeyboardInput();
void uiUpdateCursor();  // Для анимации курсора, вызывать в loop
bool uiIsKeyboardActive();
bool uiIsEnterPressed();  // Сбрасывает флаг после чтения

// Для обратной совместимости с существующим кодом
void initKeyboard();
void drawKeyboard();
bool handleKeyboardTouch(uint16_t x, uint16_t y);
void showKeyboard(const char* placeholder);
void hideKeyboard();
const char* getKeyboardInput();
void clearKeyboardInput();

#endif
