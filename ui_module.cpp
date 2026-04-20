#include "ui_module.h"
#include <TFT_eSPI.h>
#include "config.h"

extern TFT_eSPI tft;  // Объявлен в main файле


// ======================= ПРОТОТИПЫ ФУНКЦИЙ =======================
extern int currentScreen;
extern int lastScreen;
extern int vaultFrame;
extern const int bitmapStartX;
extern void drawScanlines();
extern void drawPipBoyScreen();
extern void drawPipBoyScreen1();
extern void drawPipBoyScreen2();
extern void drawPipBoyScreen3();
extern void drawPipBoyScreen4();
extern void drawButtonsScreen4();
extern void drawTabButtons();

// Состояние клавиатуры
bool keyboardActive = false;
char inputBuffer[32] = "";
int inputLength = 0;
int cursorPos = 0;
bool pcursor = true;
bool KeyboardEnter = false;
bool shiftActive = false;

// Структура клавиш


Key keys[55];
int keyCount = 0;

const char* layoutUpper[5][10] = {
  {"1", "2", "3", "4", "5", "6", "7", "8", "9", "0"},
  {"Q", "W", "E", "R", "T", "Y", "U", "I", "O", "P"},
  {"A", "S", "D", "F", "G", "H", "J", "K", "L", ";"},
  {"Z", "X", "C", "V", "B", "N", "M", ",", ".", "|"},
  {"!", "%", "#", "$", "*", "+", "=", "<", ">", "?"}
};

const char* layoutLower[5][10] = {
  {"1", "2", "3", "4", "5", "6", "7", "8", "9", "0"},
  {"q", "w", "e", "r", "t", "y", "u", "i", "o", "p"},
  {"a", "s", "d", "f", "g", "h", "j", "k", "l", ":"},
  {"z", "x", "c", "v", "b", "n", "m", ",", ".", "/"},
  {"!", "@", "#", "$", "*", "-", "_", "(", ")", "?"}
};


static void drawKey(Key* k, bool pressed);
static void redrawInputField();

void uiInit() {
  initKeyboard();
}

void initKeyboard() {
  keyCount = 0;
  int startX = KEYBOARD_X + 5;
  int startY = KEYBOARD_Y + 55;
  
  // Ряд 1: Цифры (10 клавиш)
  for (int i = 0; i < 10; i++) {
    keys[keyCount].x = startX + i * (KEY_W + KEY_GAP);
    keys[keyCount].y = startY;
    keys[keyCount].w = KEY_W;
    keys[keyCount].h = KEY_H;
    //keys[keyCount].type = (i == 11) ? 1 : 0; // Последняя = Backspace
    strcpy(keys[keyCount].label, shiftActive ? layoutUpper[0][i] : layoutLower[0][i]);
    keyCount++;
  }
  
  // Ряд 2: QWERTY (10 клавиш)
  startX = KEYBOARD_X + 5;
  startY += KEY_H + KEY_GAP + 2;
  for (int i = 0; i < 10; i++) {
    keys[keyCount].x = startX + i * (KEY_W + KEY_GAP);
    keys[keyCount].y = startY;
    keys[keyCount].w = KEY_W;
    keys[keyCount].h = KEY_H;
    keys[keyCount].type = 0;
    strcpy(keys[keyCount].label, shiftActive ? layoutUpper[1][i] : layoutLower[1][i]);
    keyCount++;
  }
  
  // Ряд 3: ASDF (10 клавиш)
  startX = KEYBOARD_X + 5;
  startY += KEY_H + KEY_GAP + 2;
  for (int i = 0; i < 10; i++) {
    keys[keyCount].x = startX + i * (KEY_W + KEY_GAP);
    keys[keyCount].y = startY;
    keys[keyCount].w = KEY_W;
    keys[keyCount].h = KEY_H;
    keys[keyCount].type = 0;
    strcpy(keys[keyCount].label, shiftActive ? layoutUpper[2][i] : layoutLower[2][i]);
    keyCount++;
  }
  
  // Ряд 4: ZXCV (10 клавиш)
  startX = KEYBOARD_X + 5;
  startY += KEY_H + KEY_GAP + 2;
  for (int i = 0; i < 10; i++) {
    keys[keyCount].x = startX + i * (KEY_W + KEY_GAP);
    keys[keyCount].y = startY;
    keys[keyCount].w = KEY_W;
    keys[keyCount].h = KEY_H;
    keys[keyCount].type = 0;
    strcpy(keys[keyCount].label, shiftActive ? layoutUpper[3][i] : layoutLower[3][i]);
    keyCount++;
  }

  // Ряд 5: символы (10 клавиш)
  startX = KEYBOARD_X + 5;
  startY += KEY_H + KEY_GAP + 2;
  for (int i = 0; i < 10; i++) {
    keys[keyCount].x = startX + i * (KEY_W + KEY_GAP);
    keys[keyCount].y = startY;
    keys[keyCount].w = KEY_W;
    keys[keyCount].h = KEY_H;
    keys[keyCount].type = 0;
    strcpy(keys[keyCount].label, shiftActive ? layoutUpper[4][i] : layoutLower[4][i]);
    keyCount++;
  }
  
  
  // ESC
  keys[keyCount].x = KEYBOARD_X + 5; //KEYBOARD_W - 50;
  keys[keyCount].y = KEYBOARD_Y + KEY_H;
  keys[keyCount].w = 45;
  keys[keyCount].h = KEY_H;
  keys[keyCount].type = 6;
  strcpy(keys[keyCount].label, "ESC");
  keyCount++;

  // Backspace
  keys[keyCount].x = KEYBOARD_X + KEYBOARD_W - 50;
  keys[keyCount].y = KEYBOARD_Y + KEY_H;//+ KEY_H + KEY_GAP;
  keys[keyCount].w = 45;
  keys[keyCount].h = KEY_H;
  keys[keyCount].type = 1;
  strcpy(keys[keyCount].label, "DEL");
  keyCount++;
  

  //shift
  keys[keyCount].x = KEYBOARD_X + 5;
  keys[keyCount].y = KEYBOARD_H - KEY_H - KEY_GAP - 4;
  keys[keyCount].w = KEY_W * 2 + KEY_GAP;
  keys[keyCount].h = KEY_H + 4;
  keys[keyCount].type = 3;
  strcpy(keys[keyCount].label, "SHIFT");
  keyCount++;
  
  // Space
  keys[keyCount].x = KEYBOARD_X + 5 + (KEY_W * 2 + KEY_GAP * 2);
  keys[keyCount].y = KEYBOARD_H - KEY_H - KEY_GAP - 4;
  keys[keyCount].w = KEY_W * 6 + KEY_GAP * 5;
  keys[keyCount].h = KEY_H + 4;
  keys[keyCount].type = 4;
  strcpy(keys[keyCount].label, "SPACE");
  keyCount++;

  // Enter
  keys[keyCount].x = KEYBOARD_X + 5 + KEY_W * 6 + KEY_GAP * 6 + ((KEY_W + KEY_GAP ) * 2);
  keys[keyCount].y = KEYBOARD_H - KEY_H - KEY_GAP - 4;
  keys[keyCount].w = KEY_W * 2 + KEY_GAP;
  keys[keyCount].h = KEY_H + 4;
  keys[keyCount].type = 2;
  strcpy(keys[keyCount].label, "ENTER");


  keyCount++;
  
  
}

void drawKeyboard() {
  tft.fillScreen(TFT_BLACK);
  
  // Фон клавиатуры
  //tft.fillRect(KEYBOARD_X, KEYBOARD_Y, KEYBOARD_W, KEYBOARD_H, TFT_BLACK);
  tft.drawRect(KEYBOARD_X, KEYBOARD_Y, KEYBOARD_W, KEYBOARD_H, TFT_GREEN);
  drawScanlines();
  // Поле ввода
  tft.drawRect(KEYBOARD_X + 55, KEYBOARD_Y + 28, KEYBOARD_W - 110, 22, TFT_GREEN);
  
  // Клавиши
  for (int i = 0; i < keyCount; i++) {
    drawKey(&keys[i], false);
  }
  
  // Текст подсказки
  tft.setTextColor(TFT_GREEN);
  tft.setTextSize(1);
  tft.setTextDatum(TC_DATUM);
  tft.setCursor(KEYBOARD_X + 8, KEYBOARD_Y + 8);
  tft.print("VAULT-TEC KEYBOARD v1.0");
  //tft.drawString("VAULT-TEC KEYBOARD v1.0", KEYBOARD_X + 8, KEYBOARD_Y + 8);
  
  redrawInputField();
}

static void drawKey(Key* k, bool pressed) {
  if (pressed) {
    tft.fillRect(k->x, k->y, k->w, k->h, TFT_GREEN);
    tft.setTextColor(TFT_BLACK);
  } else {
    tft.fillRect(k->x, k->y, k->w, k->h, TFT_BLACK);
    tft.drawRect(k->x, k->y, k->w, k->h, TFT_GREEN);
    tft.setTextColor(TFT_GREEN);
  }
  
  tft.setTextSize(1);
  tft.setTextDatum(MC_DATUM);
  tft.drawString(k->label, k->x + k->w/2, k->y + k->h/2);
}

static void redrawInputField() {
  tft.fillRect(KEYBOARD_X + 57, KEYBOARD_Y + 30, KEYBOARD_W - 115, 18, TFT_BLACK);
  tft.setTextColor(TFT_GREEN);
  tft.setTextDatum(TL_DATUM);
  tft.setTextSize(1);
  tft.drawString(inputBuffer, KEYBOARD_X + 60, KEYBOARD_Y + 39);
}

void uiDrawKeyboard() {
  drawKeyboard();
}

bool uiHandleKeyboardTouch(int x, int y) {
  return handleKeyboardTouch(x, y);
}

bool handleKeyboardTouch(uint16_t x, uint16_t y) {
  if (!keyboardActive) return false;
  int idx = 0;
  for (int i = 0; i < keyCount; i++) {
    Key* k = &keys[i];
    if (x >= k->x && x <= k->x + k->w && y >= k->y && y <= k->y + k->h) {
      // Визуальный отклик
      drawKey(k, true);
      delay(100);
      drawKey(k, false);
      
      switch (k->type) {
        case KEY_CHAR:
        {
          if (inputLength < 31) {
            for (int j = inputLength; j > cursorPos; j--) {
              inputBuffer[j] = inputBuffer[j-1];
            }
            inputBuffer[cursorPos] = k->label[0];
            inputLength++;
            cursorPos++;
            inputBuffer[inputLength] = '\0';
            redrawInputField();
          }
          break;
        }
        case KEY_BACKSPACE:
        {
          if (cursorPos > 0) {
            for (int j = cursorPos - 1; j < inputLength; j++) {
              inputBuffer[j] = inputBuffer[j+1];
            }
            inputLength--;
            cursorPos--;
            inputBuffer[inputLength] = '\0';
            redrawInputField();
          }
          break;
        }
        case KEY_ENTER:
        {
          KeyboardEnter = true;
          keyboardActive = false;
          hideKeyboard();
          return true;
          break;
        } 
        case KEY_SHIFT:
        {
          // Обновить раскладку
          shiftActive = !shiftActive;
          initKeyboard();
          drawKeyboard();
          break;
        } 
        case KEY_SPACE:
        {
          if (inputLength < 31) {
            for (int j = inputLength; j > cursorPos; j--) {
              inputBuffer[j] = inputBuffer[j-1];
            }
            inputBuffer[cursorPos] = ' ';
            inputLength++;
            cursorPos++;
            inputBuffer[inputLength] = '\0';
            redrawInputField();
          }
          break;
        }
        case KEY_ESC:
        {
          inputBuffer[0] = '\0';
          inputLength = 0;
          hideKeyboard(); 
          return true;
        }
      }
      
      //redrawInputField();
      return true;
    }
  }
  return true;
}

void showKeyboard(const char* placeholder) {
  keyboardActive = true;
  inputLength = 0;
  inputBuffer[0] = '\0';
  cursorPos = 0;
  shiftActive = false;
  
  if (placeholder) {
    strncpy(inputBuffer, placeholder, 29);
    inputLength = strlen(inputBuffer);
    cursorPos = inputLength;
  }
  
  initKeyboard();
  drawKeyboard();
}

void hideKeyboard() {
  keyboardActive = false;
  tft.fillScreen(TFT_BLACK);
  drawScanlines();
  if (currentScreen == 0) drawPipBoyScreen();
  if (currentScreen == 1) drawPipBoyScreen1();
  if (currentScreen == 2) drawPipBoyScreen2();
  if (currentScreen == 3) drawPipBoyScreen3();
  if (currentScreen == 4) drawPipBoyScreen4();
  drawTabButtons();

  //tft.fillRect(KEYBOARD_X, KEYBOARD_Y, KEYBOARD_W, KEYBOARD_H, TFT_BLACK);
}

const char* getKeyboardInput() {
  return inputBuffer;
}

void clearKeyboardInput() {
  inputBuffer[0] = '\0';
  inputLength = 0;
}

void uiShowKeyboard(const char* placeholder) {
  showKeyboard(placeholder);
}

void uiHideKeyboard() {
  hideKeyboard();
}

const char* uiGetKeyboardInput() {
  return getKeyboardInput();
}

void uiClearKeyboardInput() {
  clearKeyboardInput();
}

void uiUpdateCursor() {
  if (keyboardActive) {
    static unsigned long lastCursor = 0;
    if (millis() - lastCursor > 500) {
      lastCursor = millis();
      pcursor = !pcursor;
      redrawInputField();
    }
  }
}

bool uiIsKeyboardActive() {
  return keyboardActive;
}

bool uiIsEnterPressed() {
  if (KeyboardEnter) {
    KeyboardEnter = false;
    return true;
  }
  return false;
}
