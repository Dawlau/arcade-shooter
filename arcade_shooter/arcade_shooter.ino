#include "LedControl.h"
#include <LiquidCrystal.h>
#include "EEPROM.h"

// TO DO:
// fix highscore
// add physics (jump)

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

const byte selectionArrow[8] = {
  B01000,
  B00100,
  B00010,
  B00001,
  B00010,
  B00100,
  B01000,
  B10000
};

const byte increaseArrow[8] = {
  B01000,
  B01100,
  B01110,
  B01111,
  B01110,
  B01100,
  B01100,
  B01000
};

const byte decreaseArrow[8] = {
  B00010,
  B00110,
  B01110,
  B11110,
  B01110,
  B00110,
  B00010,
  B00000
};

const int downScrollingArrowLcdId = 0;
const int upScrollingArrowLcdId = 1;
const int selectionArrowLcdId = 2;
const int decreaseArrowLcdId = 3;
const int increaseArrowLcdId = 4;

const int RS = 8;
const int enable = 9;
const int D4 = 7;
const int D5 = 13;
const int D6 = 5;
const int D7 = 4;

const int dinPin = 12;
const int clockPin = 11;
const int loadPin = 10;

const int joystickPinX = A0;
const int joystickPinY = A1;
const int joystickPinSW = 2;

const int brightnessPin = 6;

const int matrixSize = 8;

const int joystickMinThreshold = 200;
const int joystickMaxThreshold = 750;

const int contrastPin = 3;

const String gameName = "Arcade-shooter";
const String defaultName = "anonymous";
const int nameSize = 9;

const int maxHighscoresCount = 3;

const int minLevel = 1;
const int maxLevel = 6;

String highscoreNames[maxHighscoresCount];

byte highscores[maxHighscoresCount];

int xValue = 0;
int yValue = 0;

byte xPos = 0;
byte yPos = 0;

byte xLastPos = 0;
byte yLastPos = 0;

const int minMatrixBrightness = 1;
int matrixBrightness = 8;
const int maxMatrixBrightness = 9;

const int minBrightness = 10;
int brightness = 240;
const int maxBrightness = 245;
const int brightnessOffset = 10;


const int lcdWidth = 16;
const int lcdHeight = 2;

unsigned long long lastMoved = 0;
const int moveInterval = 100;

const unsigned int minContrast = 60;
unsigned int contrast = 110;
const unsigned int maxContrast = 150;

bool matrixUpdate = true;
int level = minLevel;

String playerName = defaultName;
int playerNameIndex = 0;
int playerHighscore = 4;

const byte contrastAddress = 0;
const byte brightnessAddress = 1;
const byte matrixBrightnessAddress = 2;
const byte highscoreAddress = 3;

const byte mapHeight = 8;
const byte mapWidth = 50;

bool gameMap[mapHeight][mapWidth];
const byte minPlatformLength = 3;
const byte maxPlatformLength = 6;
const byte characterSize = 3; // size of player and enemies

bool fovUpdate;

byte cameraLeftPosition = 0;
byte cameraRightPosition = matrixSize;

const byte jumpSize = 3;

byte playerRow = 0;
byte playerCol = 0;

byte jumpsLeft = 0;

LedControl lc = LedControl(dinPin, clockPin, loadPin, 1); //DIN, CLK, LOAD, No. DRIVER
LiquidCrystal lcd(RS, enable, D4, D5, D6, D7);

enum state {
  unknown, // mostly for debugging
  welcomeScreen,
  homeScreen,
  highscore,
  about,
  settings,
  play,
  changeName,
  setLevel,
  setContrast,
  setBrightness,
  setMatrixBrightness
};

enum joystickMove {
  up,
  down,
  left,
  right,
  neutral,
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

  pinMode(brightnessPin, OUTPUT);

  contrast = EEPROM.read(contrastAddress);
  brightness = EEPROM.read(brightnessAddress);
  matrixBrightness = EEPROM.read(matrixBrightnessAddress);


  analogWrite(contrastPin, contrast);
  analogWrite(brightnessPin, brightness);

  lc.shutdown(0, false);
  lc.setIntensity(0, 15);
  lc.clearDisplay(0);

  gameState = welcomeScreen;
  lcd.begin(lcdWidth, lcdHeight);
  lcd.createChar(downScrollingArrowLcdId, downScrollingArrow);
  lcd.createChar(upScrollingArrowLcdId, upScrollingArrow);
  lcd.createChar(selectionArrowLcdId, selectionArrow);
  lcd.createChar(increaseArrowLcdId, increaseArrow);
  lcd.createChar(decreaseArrowLcdId, decreaseArrow);

  for (int i = 0, address = highscoreAddress; i < maxHighscoresCount; i++, address++) {
    String copyName = "";
    for (int characterIndex = 0; characterIndex < nameSize; characterIndex++) {
      copyName += char(EEPROM.read(address));
      address++;
    }
    highscoreNames[i] = copyName;
    highscores[i] = EEPROM.read(address);
  }

  for (int i = 0; i < 3; i++ ) {
    Serial.println(highscoreNames[i]);
  }

  randomSeed(analogRead(0));


  //  for (int i = 0, address = highscoreAddress; i < maxHighscoresCount; i++, address++) {
  //    for (int characterIndex = 0; characterIndex < nameSize; characterIndex++) {
  //      EEPROM.update(address, 0);
  //      address++;
  //    }
  //    EEPROM.update(address, 0);
  //  }
  //
  //  for(int i = 0;i < 3; i++) {
  //    highscores[i] = 3 - i;
  //    highscoreNames[i] = defaultName;
  //  }
}

void generateLevel() {

  byte lastPlatformHeight = 3;
  byte lastPlatformEnd = -1;

  for (int col = 0; col < mapWidth;) {

    bool startPlatform; // 0 or 1

    if (col - lastPlatformEnd > jumpSize) {
      startPlatform = true;
    }
    else {
      startPlatform = random(2);
    }

    if (startPlatform) {
      byte platformLength = random(minPlatformLength, min(mapWidth - col, maxPlatformLength));
      byte mapRow = random(max(lastPlatformHeight - jumpSize, characterSize + 1), mapHeight);
      for (int i = col; i < col + platformLength; i++) {
        gameMap[mapRow][i] = true;
      }
      col += platformLength;
      lastPlatformHeight = mapRow;
      lastPlatformEnd = col - 1;
    }
    else {
      col++;
    }
  }
}

void displayMap() {

  lc.clearDisplay(0);
  for (int row = 0; row < matrixSize; row++) {
    for (int col = cameraLeftPosition; col < cameraRightPosition; col++) {
      lc.setLed(0, matrixSize - (col - cameraLeftPosition) - 1, row, gameMap[row][col]);
    }
  }

  for (int row = playerRow; row > playerRow - characterSize; row--) {
    lc.setLed(0, matrixSize - (playerCol - cameraLeftPosition) - 1, row, true);
  }

  lc.setLed(0, matrixSize - (playerCol + 1 - cameraLeftPosition) - 1, playerRow - 1, true); // display hand

  //  delay(1000);
}

void setStartingPosition() {

  for (int col = 0; col < mapWidth; col++) {
    for (int row = 0; row < mapHeight; row++) {
      if (gameMap[row][col]) {
        playerRow = row - 1;
        playerCol = col;

        Serial.print(playerRow);
        Serial.print(' ');
        Serial.println(playerCol);

        return ;
      }
    }
  }

}

bool checkCollision(int nextRow, int nextCol) {

  for (int row = nextRow; row > playerRow - characterSize; row--) {
    if (gameMap[row][nextCol]) {
      return true;
    }
  }

  if (gameMap[nextRow - 1][nextCol + 1]) {
    return true;
  }

  return false;
}

void runPlayGame() {

  static bool startOfLevel = true;
  static unsigned long long lastJump = 0;
  static int jumpInterval = 200;
  static int fallInterval = 200;
  static unsigned long long lastFall = 0;

  if (startOfLevel) {
    for (int row = 0; row < mapHeight; row++) {
      for (int col = 0; col < mapWidth; col++) {
        gameMap[row][col] = false;
      }
    }
    generateLevel();
    setStartingPosition();
    startOfLevel = false;
    fovUpdate = true;
  }

  if (jumpsLeft) {
    if (playerRow - characterSize + 1 > 0) {
      if (millis() - lastJump >= jumpInterval) {
        jumpsLeft--;
        playerRow--;
        lastJump = millis();
      }
    }
    else {
      jumpsLeft = 0;
    }
  }

  int joystickMove = joystickVerticalMove();

  if (joystickMove == up && (checkCollision(playerRow + 1, playerCol) || checkCollision(playerRow, playerCol + 1))) {
    jumpsLeft = jumpSize;
  }

  if (playerRow == mapHeight) { // dead
    startOfLevel = true;
    gameState = homeScreen;
    cameraLeftPosition = 0;
    cameraRightPosition = matrixSize;
    return ;
  }

  if (!checkCollision(playerRow + 1, playerCol) && jumpsLeft == 0 && millis() - lastFall > fallInterval) { // go down if there is no platform
    playerRow++;
    lastFall = millis();
    fovUpdate = true;
  }

  joystickMove = joystickHorizontalMove();

  if (joystickMove == left && cameraLeftPosition > 0 && !checkCollision(playerRow, playerCol - 1)) {
    cameraLeftPosition--;
    cameraRightPosition--;
    playerCol--;
    fovUpdate = true;
  }

  if (joystickMove == right && cameraRightPosition < mapWidth && !checkCollision(playerRow, playerCol + 1)) {
    cameraLeftPosition++;
    cameraRightPosition++;
    playerCol++;
    fovUpdate = true;
  }

  if (fovUpdate) {
    displayMap();
  }
}

void loop() {

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
  else if (gameState == changeName) {
    runChangeName();
  }
  else if (gameState == setLevel) {
    runSetLevel();
  }
  else if (gameState == setContrast) {
    runSetContrast();
  }
  else if (gameState == setBrightness) {
    runSetBrightness();
  }
  else if (gameState == setMatrixBrightness) {
    runSetMatrixBrightness();
  }
  else if (gameState == play) {
    runPlayGame();
  }

  lastJoystickSwState = joystickSwState;
}

void runWelcomeScreen() {

  static const int welcomeScreenDuration = 1000;
  static bool displayed = false;

  if (millis() < welcomeScreenDuration) { // enough to just check millis() against the duration since it is the first thing that runs
    if (!displayed) {
      lcd.clear();
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

joystickMove joystickVerticalMove() {

  static int joystickMoved = false;

  int xValue = analogRead(joystickPinX);

  if (xValue < joystickMinThreshold && !joystickMoved) { // joystick moved up
    joystickMoved = true;
    return up;
  }

  if (xValue > joystickMaxThreshold && !joystickMoved) { // joystick moved down
    joystickMoved = true;
    return down;
  }

  if (xValue >= joystickMinThreshold && xValue <= joystickMaxThreshold) {
    joystickMoved = false;
  }

  return neutral;
}


int renderScrollingMenu(String contents[], int contentsLength, bool useSelection = false) {

  static int menuRow = 0;
  static int lastMenuRow = 1;
  static int selectedRow = 0;
  static int lastSelectedRow = 0;

  int joystickMove = joystickVerticalMove();

  if (joystickMove == up) {
    if (menuRow) {
      menuRow--;
    }
    if (selectedRow) {
      selectedRow--;
    }
  }
  else if (joystickMove == down) {
    if (menuRow < contentsLength - lcdHeight) {
      menuRow++;
    }
    if (selectedRow < contentsLength - 1) {
      selectedRow++;
    }
  }

  if (menuRow != lastMenuRow || selectedRow != lastSelectedRow) {

    lcd.clear();

    for (int i = menuRow; i < menuRow + lcdHeight; i++) {
      lcd.setCursor(0, i - menuRow);
      lcd.print(contents[i]);

      if (useSelection && i == selectedRow) {
        lcd.setCursor(contents[i].length() + 1, i - menuRow);
        lcd.write(byte(selectionArrowLcdId));
      }
    }

    if (menuRow) {
      lcd.setCursor(lcdWidth - 1, 0);
      lcd.write(byte(upScrollingArrowLcdId));
    }

    if (menuRow < contentsLength - lcdHeight) {

      lcd.setCursor(lcdWidth - 1, lcdHeight - 1);
      lcd.write(byte(downScrollingArrowLcdId));
    }
  }

  lastMenuRow = menuRow;
  lastSelectedRow = selectedRow;

  if (joystickSwState != lastJoystickSwState) {
    menuRow = 0;
    lastMenuRow = 1;
    lastSelectedRow = 0;

    int auxSelectedRow = selectedRow;
    selectedRow = 0;

    if (useSelection) {
      return auxSelectedRow;
    }
    else {
      gameState = homeScreen;
      return -1; // code for "don't load anything else"
    }
  }

  return -1;
}


void runHomeScreen() {

  static const int optionsLength = 4;
  static const String options[] = {
    "1.Play",
    "2.Settings",
    "3.Highscores",
    "4.About"
  };

  int exitCode = renderScrollingMenu(options, optionsLength, true);

  if (exitCode != -1) {
    exitCode++;
    if (exitCode == 1) {
      gameState = play;
    }
    else if (exitCode == 2) {
      gameState = settings;
    }
    else if (exitCode == 3) {
      gameState = highscore;
    }
    else {
      gameState = about;
    }
  }
}

void runAboutMenu() {

  static const int aboutDescriptionRowsCount = 8;
  static const String aboutDescription[aboutDescriptionRowsCount] = {
    "Arcade-shooter",
    "Made by",
    "Andrei Blahovici",
    "Github link:",
    "github.com/",
    "Dawlau/",
    "arcade-shooter",
    "-Press to back-"
  };

  int exitCode = renderScrollingMenu(aboutDescription, aboutDescriptionRowsCount);
}

void runSettingsMenu() {

  static const int settingsCount = 6;
  static String settings[settingsCount] = {
    "1.Enter name",
    "2.Level",
    "3.Contrast",
    "4.Brightness",
    "5.Game light",
    "6.Back"
  };

  int exitCode = renderScrollingMenu(settings, settingsCount, true);

  if (exitCode != -1) {
    exitCode++;
  }

  if (exitCode == 6) {
    gameState = homeScreen;
  }
  else if (exitCode == 1) {
    gameState = changeName;
  }
  else if (exitCode == 2) {
    gameState = setLevel;
  }
  else if (exitCode == 3) {
    gameState = setContrast;
  }
  else if (exitCode == 4) {
    gameState = setBrightness;
  }
  else if (exitCode == 5) {
    gameState = setMatrixBrightness;
  }
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

  static const int lcdContentsLength = maxHighscoresCount + 1;
  static String lcdContents[lcdContentsLength];

  if (lcdContents[0] == "") {
    for (int i = 0;  i < maxHighscoresCount; i++) {
      String spaces = "";
      int spacesCount = lcdWidth - digitsCount(highscores[i]) - 1 - highscoreNames[i].length();

      while (spacesCount) {
        spaces += " ";
        spacesCount--;
      }

      lcdContents[i] = highscoreNames[i] + spaces + String(highscores[i]);
    }

    lcdContents[lcdContentsLength - 1] = "-Press to back-";
  }

  int exitCode = renderScrollingMenu(lcdContents, lcdContentsLength);
}

joystickMove joystickHorizontalMove() {

  static unsigned long long lastChange = 0;
  static unsigned int updateInterval = 200;

  if (millis() - lastChange > updateInterval) {


    int yValue = analogRead(joystickPinY);

    if (yValue < joystickMinThreshold) { // joystick moved right
      lastChange = millis();
      return right;
    }

    if (yValue > joystickMaxThreshold) { // joystick moved left
      lastChange = millis();
      return left;
    }
  }

  return neutral;
}

void runChangeName() {

  static int lastPlayerNameIndex = 1;
  static bool changedLetter = false;

  int joystickMoveHorizontal = joystickHorizontalMove();

  if (joystickMoveHorizontal == left && playerNameIndex > 0) {
    playerNameIndex--;
  }

  if (joystickMoveHorizontal == right && playerNameIndex < nameSize - 1) {
    playerNameIndex++;
  }

  int joystickMoveVertical = joystickVerticalMove();

  if (joystickMoveVertical == up) {
    changedLetter = true;
    if (playerName[playerNameIndex] == 'z') {
      playerName[playerNameIndex] = ' ';
    }
    else if (playerName[playerNameIndex] == ' ') {
      playerName[playerNameIndex] = 'a';
    }
    else {
      playerName[playerNameIndex]++;
    }
  }

  if (joystickMoveVertical == down) {
    changedLetter = true;
    if (playerName[playerNameIndex] == ' ') {
      playerName[playerNameIndex] = 'z';
    }
    else if (playerName[playerNameIndex] == 'a') {
      playerName[playerNameIndex] = ' ';
    }
    else {
      playerName[playerNameIndex]--;
    }
  }

  if (lastPlayerNameIndex != playerNameIndex || changedLetter) {

    changedLetter = false;
    lcd.clear();

    String title = "Name: (X=space)";

    lcd.home();
    lcd.print(title);

    for (int i = 0, lcdPosition = (lcdWidth - nameSize) / 2; i < nameSize; i++, lcdPosition++) {
      lcd.setCursor(lcdPosition, 1);
      if (playerName[i] == ' ') {
        lcd.print('X');
      }
      else {
        lcd.print(playerName[i]);
      }
    }

    lcd.setCursor((lcdWidth - nameSize) / 2 + playerNameIndex, 1);
    lcd.cursor();
  }

  lastPlayerNameIndex = playerNameIndex;


  if (joystickSwState != lastJoystickSwState) {
    lastPlayerNameIndex = 1;
    changedLetter = false;
    gameState = settings;
    lcd.noCursor();
  }
}

void runSetLevel() {

  static int lastLevel = 0;

  int joystickMove = joystickHorizontalMove();

  if (joystickMove == left && level > minLevel) {
    level--;
  }

  if (joystickMove == right && level < maxLevel) {
    level++;
  }


  String title = "Select level:";

  if (level != lastLevel) {
    lcd.clear();

    lcd.home();
    lcd.print(title);

    lcd.setCursor(lcdWidth / 2 - 2, 1);
    lcd.write(byte(decreaseArrowLcdId));

    lcd.setCursor(lcdWidth / 2 + 2, 1);
    lcd.write(byte(increaseArrowLcdId));

    lcd.setCursor(lcdWidth / 2, 1);
    lcd.print(level);
  }

  lastLevel = level;

  if (joystickSwState != lastJoystickSwState) {
    lastLevel = 0;
    gameState = settings;
  }
}

void runSetContrast() {

  static int lastContrast = 0;

  int joystickMove = joystickHorizontalMove();

  if (joystickMove == left && contrast > minContrast) {
    contrast--;
  }

  if (joystickMove == right && contrast < maxContrast) {
    contrast++;
  }

  String title = "Select contrast:";

  if (contrast != lastContrast) {
    lcd.clear();

    lcd.home();
    lcd.print(title);

    lcd.setCursor(lcdWidth / 2 - 2, 1);
    lcd.write(byte(decreaseArrowLcdId));

    lcd.setCursor(lcdWidth / 2 + 2, 1);
    lcd.write(byte(increaseArrowLcdId));

    lcd.setCursor(lcdWidth / 2, 1);
    lcd.print(contrast - minContrast);

    analogWrite(contrastPin, contrast);
  }

  lastContrast = contrast;

  if (joystickSwState != lastJoystickSwState) {
    lastContrast = 0;
    EEPROM.update(contrastAddress, contrast);
    gameState = settings;
  }
}

void runSetBrightness() {

  static int lastBrightness = 0;

  int joystickMove = joystickHorizontalMove();

  if (joystickMove == left && brightness > minBrightness) {
    brightness -= brightnessOffset;
  }

  if (joystickMove == right && brightness < maxBrightness) {
    brightness += brightnessOffset;
  }

  String title = "Brightness:";

  if (brightness != lastBrightness) {
    lcd.clear();

    lcd.home();
    lcd.print(title);

    int startPosition = lcdWidth - 4 - digitsCount(brightness - minBrightness);
    startPosition /= 2;

    lcd.setCursor(startPosition, 1);
    lcd.write(byte(decreaseArrowLcdId));

    lcd.setCursor(startPosition + digitsCount(brightness - minBrightness) + 3, 1);
    lcd.write(byte(increaseArrowLcdId));

    lcd.setCursor(startPosition + 2, 1);
    lcd.print(brightness - minBrightness);

    analogWrite(brightnessPin, brightness);
  }

  lastBrightness = brightness;

  if (joystickSwState != lastJoystickSwState) {
    lastBrightness = 0;
    EEPROM.update(brightnessAddress, brightness);
    gameState = settings;
  }
}

void runSetMatrixBrightness() {

  static int lastMatrixBrightness = 0;

  int joystickMove = joystickHorizontalMove();

  if (joystickMove == left && matrixBrightness > minMatrixBrightness) {
    matrixBrightness--;
  }

  if (joystickMove == right && matrixBrightness < maxMatrixBrightness) {
    matrixBrightness++;
  }

  String title = "Game brightness:";

  if (matrixBrightness != lastMatrixBrightness) {
    lcd.clear();

    lcd.home();
    lcd.print(title);

    lcd.setCursor(lcdWidth / 2 - 2, 1);
    lcd.write(byte(decreaseArrowLcdId));

    lcd.setCursor(lcdWidth / 2 + 2, 1);
    lcd.write(byte(increaseArrowLcdId));

    lcd.setCursor(lcdWidth / 2, 1);
    lcd.print(matrixBrightness);

    lc.setIntensity(0, matrixBrightness);
  }

  lastMatrixBrightness = matrixBrightness;

  if (joystickSwState != lastJoystickSwState) {
    lastMatrixBrightness = 0;
    EEPROM.update(matrixBrightnessAddress, matrixBrightness);
    gameState = settings;
  }
}

void updateHighScores() {

  // insert highscore

  for (int i = 0; i < maxHighscoresCount; i++) {
    if (highscores[i] < playerHighscore) {
      for (int j = maxHighscoresCount - 1; j > i; j--) {
        highscores[j] = highscores[j - 1];
        highscoreNames[j] = highscoreNames[j - 1];
      }
      highscores[i] = playerHighscore;
      highscoreNames[i] = playerName;
      break;
    }
  }

  for (int i = 0, address = highscoreAddress; i < maxHighscoresCount; i++, address++) {
    for (int characterIndex = 0; characterIndex < nameSize; characterIndex++) {
      EEPROM.update(address, highscoreNames[i][characterIndex]);
      address++;
    }
    EEPROM.update(address, highscores[i]);
  }
}
