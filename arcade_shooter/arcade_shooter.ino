// TO DO:
// create a scrolling text function
// maybe delete the write text at pos function 

#include "LedControl.h"
#include <LiquidCrystal.h>

const byte downScrollingArrow[8] = {
  B00000,
  B00000,
  B00000,
  B00000,
  B00000,
  B11111,
  B01110,
  B00100
};

const byte upScrollingArrow[8] = {
  B00100,
  B01110,
  B11111,
  B00000,
  B00000,
  B00000,
  B00000,
  B00000
};

const int downScrollingArrowLcdId = 0;
const int upScrollingArrowLcdId = 1;

const int RS = 8;
const int enable = 9;
const int D4 = 7;
const int D5 = 6;
const int D6 = 5;
const int D7 = 4;

const int dinPin = 12;
const int clockPin = 11;
const int loadPin = 10;

const int joystickPinX = A0;
const int joystickPinY = A1;
const int joystickPinSW = 2;

const int matrixSize = 8;

const int joystickMinThreshold = 200;
const int joystickMaxThreshold = 850;

const int contrastPin = 3;

const String gameName = "Arcade-shooter";

const int maxHighscoresCount = 3;

String highscoreNames[maxHighscoresCount] = {
  "aaaa",
  "bbbb",
  "cccc",
};

int highscores[maxHighscoresCount] = {
  1523, 110, 12
};

int xValue = 0;
int yValue = 0;

byte xPos = 0;
byte yPos = 0;

byte xLastPos = 0;
byte yLastPos = 0;

int matrixBrightness = 15;


const int lcdWidth = 16;
const int lcdHeight = 2;

unsigned long long lastMoved = 0;
const int moveInterval = 100;

unsigned int contrast = 110;

bool matrixUpdate = true;

LedControl lc = LedControl(dinPin, clockPin, loadPin, 1); //DIN, CLK, LOAD, No. DRIVER
LiquidCrystal lcd(RS, enable, D4, D5, D6, D7);

enum state {
  unknown, // mostly for debugging
  welcomeScreen,
  homeScreen,
  highscore,
  about,
  settings,
  play
};

state gameState;

bool joystickSwState = LOW;
bool lastJoystickSwState = LOW;

bool matrix[matrixSize][matrixSize] = {
  {1, 0, 0, 0, 0, 0, 0, 0},
  {0, 0, 0, 0, 1, 0, 0, 0},
  {0, 0, 0, 0, 0, 0, 0, 0},
  {0, 0, 0, 0, 1, 0, 0, 0},
  {0, 0, 0, 0, 0, 0, 0, 0},
  {0, 0, 0, 0, 0, 0, 0, 0},
  {0, 0, 0, 0, 0, 0, 0, 0},
  {0, 0, 0, 0, 0, 0, 0, 0}
};

void changeJoystickSwState() {

  static const int debounceInterval = 200;
  static unsigned long long lastInterruptTime = 0;

  unsigned long long interruptTime = millis();

  if (interruptTime - lastInterruptTime >= debounceInterval) {
    joystickSwState = !joystickSwState;
  }

  lastInterruptTime = interruptTime;
}


void setup() {
  Serial.begin(9600);

  pinMode(joystickPinSW, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(joystickPinSW), changeJoystickSwState, FALLING); // set interrupt for joystick button

  pinMode(joystickPinX, INPUT);
  pinMode(joystickPinY, INPUT);

  pinMode(contrastPin, OUTPUT);

  gameState = welcomeScreen;
  lcd.begin(lcdWidth, lcdHeight);

  lcd.createChar(downScrollingArrowLcdId, downScrollingArrow);
  lcd.createChar(upScrollingArrowLcdId, upScrollingArrow);
}


void loop() {

  analogWrite(contrastPin, contrast);

  Serial.println(gameState);

  if (gameState == welcomeScreen) {
    runWelcomeScreen();
  }
  else if (gameState == homeScreen) {
    runHomeScreen();
  }
  else if (gameState == highscore) {
    runHighscoreMenu();
  }
  else if (gameState == settings) {
    runSettingsMenu();
  }
  else if (gameState == about) {
    runAboutMenu();
  }
  else {

  }

  lastJoystickSwState = joystickSwState;
}

void runWelcomeScreen() {

  static const int welcomeScreenDuration = 1000;
  static bool displayed = false;

  if (millis() < welcomeScreenDuration) { // enough to just check millis() against the duration since it is the first thing that runs
    if (!displayed) {
      lcd.home(); // first row
      lcd.print("Welcome to");
      lcd.setCursor(0, lcdHeight - 1); // second row
      lcd.print(gameName + "!");
      displayed = true;
    }
  }
  else {
    lcd.clear();
    gameState = homeScreen;
  }

}

void lcdPrintTextAtPosition(String text, int row, int col) {

  lcd.setCursor(col, row);
  lcd.print(text);
}

void updateSelectedOption(int& selectedRow, int& selectedCol) {

  static int joystickMoved = false;
  static const int menuSize = 2;

  int xValue = analogRead(joystickPinX);
  int yValue = analogRead(joystickPinY);

  if (xValue < joystickMinThreshold && !joystickMoved) { // joystick moved up
    if (selectedRow > 0) {
      selectedRow--;
      joystickMoved = true;
    }
  }

  if (xValue > joystickMaxThreshold && !joystickMoved) { // joystick moved down
    if (selectedRow < menuSize - 1) {
      selectedRow++;
      joystickMoved = true;
    }
  }

  if (yValue < joystickMinThreshold && !joystickMoved) { // joystick moved right
    if (selectedCol < menuSize - 1) {
      selectedCol++;
      joystickMoved = true;
    }
  }

  if (yValue > joystickMaxThreshold && !joystickMoved) { // joystick moved left
    if (selectedCol > 0) {
      selectedCol--;
      joystickMoved = true;
    }
  }

  if ((xValue >= joystickMinThreshold && xValue <= joystickMaxThreshold) && (yValue >= joystickMinThreshold && yValue <= joystickMaxThreshold)) {
    joystickMoved = false;
  }

}

void runHomeScreen() {

  static const int menuSize = 2;
  static const String menu[menuSize][menuSize] = {
    {"Play", "Highscore"},
    {"Settings", "About"}
  };

  static const int spaceBetweenOptions = 2;
  static const String selectionCharacter = ".";

  static bool selected[menuSize][menuSize] = {
    {true, false},
    {false, false}
  };

  static int selectedRow = 0;
  static int selectedCol = 0;

  static int lastSelectedRow = 1;
  static int lastSelectedCol = 0;

  int lcdRow = 0;
  int lcdCol = 0;

  selected[selectedRow][selectedCol] = false;
  updateSelectedOption(selectedRow, selectedCol);
  selected[selectedRow][selectedCol] = true;

  if (lastSelectedRow != selectedRow || lastSelectedCol != selectedCol) { // refresh lcd if the selected option has changed
    lcd.clear();

    for (int row = 0; row < menuSize; row++) {
      lcdRow = row;
      for (int col = 0; col < menuSize; col++) {
        lcdPrintTextAtPosition(menu[row][col], lcdRow, lcdCol);
        lcdCol += menu[row][col].length();

        if (selected[row][col]) {
          lcdPrintTextAtPosition(selectionCharacter, lcdRow, lcdCol);
        }
        lcdCol += spaceBetweenOptions; // change this
      }
      lcdCol = 0;
    }
  }

  lastSelectedRow = selectedRow;
  lastSelectedCol = selectedCol;

  if (joystickSwState != lastJoystickSwState) {
    if (menu[selectedRow][selectedCol] == "Play") {
      gameState = play;
    }
    else if (menu[selectedRow][selectedCol] == "Highscore") {
      gameState = highscore;
    }
    else if (menu[selectedRow][selectedCol] == "Settings") {
      gameState = settings;
    }
    else if (menu[selectedRow][selectedCol] == "About") {
      gameState = about;
    }
    else {
      gameState = unknown;
    }
    selected[selectedRow][selectedCol] = false;
    selected[0][0] = true;
    selectedRow = selectedCol = 0;
    lastSelectedRow = 1;
    lastSelectedCol = 0;

  }
}

void runAboutMenu() {

  static const int aboutDescriptionRowsCount = 7;
  static const String aboutDescription[aboutDescriptionRowsCount] = {
    "Arcade-shooter",
    "Made by",
    "Andrei Blahovici",
    "Github link:",
    "github.com/",
    "Dawlau/",
    "arcade-shooter"
  };

  static int menuRow = 0;
  static int lastMenuRow = 1;

  static bool joystickMoved = false;

  int xValue = analogRead(joystickPinX);

  if (xValue < joystickMinThreshold && !joystickMoved) { // joystick moved up
    if (menuRow) {
      menuRow--;
      joystickMoved = true;
    }
  }

  if (xValue > joystickMaxThreshold && !joystickMoved) { // joystick moved down
    if (menuRow < aboutDescriptionRowsCount - lcdHeight + 1) {
      menuRow++;
      joystickMoved = true;
    }
  }

  if (xValue >= joystickMinThreshold && xValue <= joystickMaxThreshold) {
    joystickMoved = false;
  }

  if (menuRow != lastMenuRow) {

    lcd.clear();

    if (menuRow == aboutDescriptionRowsCount - lcdHeight + 1) {

      for (int i = menuRow; i < aboutDescriptionRowsCount; i++) {
        lcd.setCursor(0, i - menuRow);
        lcd.print(aboutDescription[i]);
        lcd.setCursor(lcdWidth - 1, i - menuRow);
        lcd.write(byte(upScrollingArrowLcdId));
      }

      lcd.setCursor(0, lcdHeight - 1);
      lcd.print("-Press to back-");
    }
    else {
      for (int i = menuRow; i < menuRow + lcdHeight; i++) {
        lcd.setCursor(0, i - menuRow);
        lcd.print(aboutDescription[i]);
      }

      if (menuRow) {
        lcd.setCursor(lcdWidth - 1, 0);
        lcd.write(byte(upScrollingArrowLcdId));
      }

      lcd.setCursor(lcdWidth - 1, lcdHeight - 1);
      lcd.write(byte(downScrollingArrowLcdId));
    }
  }

  if (joystickSwState != lastJoystickSwState) {
    menuRow = 0;
    lastMenuRow = 1;
    joystickMoved = false;
    gameState = homeScreen;
  }

  lastMenuRow = menuRow;
}

void runSettingsMenu() {

}

int digitsCount(int no) {

  int count = 1;

  while (no > 9) {
    count++;
    no /= 10;
  }

  return count;
}

void runHighscoreMenu() {

  static int menuRow = 0;
  static int lastMenuRow = 1;
  static bool joystickMoved = false;

  int xValue = analogRead(joystickPinX);

  if (xValue < joystickMinThreshold && !joystickMoved) { // joystick moved up
    if (menuRow) {
      menuRow--;
      joystickMoved = true;
    }
  }

  if (xValue > joystickMaxThreshold && !joystickMoved) { // joystick moved down
    if (menuRow < maxHighscoresCount - lcdHeight + 1) {
      menuRow++;
      joystickMoved = true;
    }
  }

  if (xValue >= joystickMinThreshold && xValue <= joystickMaxThreshold) {
    joystickMoved = false;
  }

  if (menuRow != lastMenuRow) {

    lcd.clear();

    if (menuRow == maxHighscoresCount - lcdHeight + 1) {

      for (int i = menuRow; i < maxHighscoresCount; i++) {
        lcd.setCursor(0, i - menuRow);
        lcd.print(highscoreNames[i]);
        lcd.setCursor(lcdWidth - digitsCount(highscores[i]) - 1, i - menuRow);
        lcd.print(highscores[i]);
        lcd.setCursor(lcdWidth - 1, i - menuRow);
        lcd.write(byte(upScrollingArrowLcdId));
      }

      lcd.setCursor(0, lcdHeight - 1);
      lcd.print("-Press to back-");
    }
    else {
      for (int i = menuRow; i < menuRow + lcdHeight; i++) {
        lcd.setCursor(0, i - menuRow);
        lcd.print(highscoreNames[i]);
        lcd.setCursor(lcdWidth - digitsCount(highscores[i]) - 1, i - menuRow);
        lcd.print(highscores[i]);
      }

      if (menuRow) {
        lcd.setCursor(lcdWidth - 1, 0);
        lcd.write(byte(upScrollingArrowLcdId));
      }

      lcd.setCursor(lcdWidth - 1, lcdHeight - 1);
      lcd.write(byte(downScrollingArrowLcdId));
    }
  }

  lastMenuRow = menuRow;

  if (joystickSwState != lastJoystickSwState) {
    menuRow = 0;
    lastMenuRow = 1;
    joystickMoved = false;
    gameState = homeScreen;
  }
}
