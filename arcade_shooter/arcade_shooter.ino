#include "LedControl.h"
#include <LiquidCrystal.h>
#include "EEPROM.h"

const byte lcdCharacterSize = 8;

const byte downScrollingArrow[lcdCharacterSize] = {
  B00000,
  B00000,
  B00000,
  B00000,
  B00000,
  B11111,
  B01110,
  B00100
};

const byte upScrollingArrow[lcdCharacterSize] = {
  B00100,
  B01110,
  B11111,
  B00000,
  B00000,
  B00000,
  B00000,
  B00000
};

const byte selectionArrow[lcdCharacterSize] = {
  B01000,
  B00100,
  B00010,
  B00001,
  B00010,
  B00100,
  B01000,
  B10000
};

const byte increaseArrow[lcdCharacterSize] = {
  B01000,
  B01100,
  B01110,
  B01111,
  B01110,
  B01100,
  B01100,
  B01000
};

const byte decreaseArrow[lcdCharacterSize] = {
  B00010,
  B00110,
  B01110,
  B11110,
  B01110,
  B00110,
  B00010,
  B00000
};

const byte downScrollingArrowLcdId = 0;
const byte upScrollingArrowLcdId = 1;
const byte selectionArrowLcdId = 2;
const byte decreaseArrowLcdId = 3;
const byte increaseArrowLcdId = 4;

const byte buzzerPin = 6;

const byte RS = 8;
const byte enable = 3;
const byte D4 = 7;
const byte D5 = 13;
const byte D6 = A2;
const byte D7 = 4;

const byte dinPin = 12;
const byte clockPin = 11;
const byte loadPin = 10;

const byte joystickPinX = A0;
const byte joystickPinY = A5;
const byte joystickPinSW = 2;

const byte brightnessPin = 5;

const byte matrixSize = 8;

const byte joystickMinThreshold = 250;
const int joystickMaxThreshold = 800;

const byte contrastPin = 9;

const String gameName = "Arcade-shooter";
const String defaultName = "anonymous";
const byte nameSize = 9;

const byte maxHighscoresCount = 3;

const byte minLevel = 1;
const byte maxLevel = 4;

String highscoreNames[maxHighscoresCount];

byte highscores[maxHighscoresCount];

const byte minMatrixBrightness = 1;
byte matrixBrightness = 8;
const byte maxMatrixBrightness = 9;

const int minBrightness = 0;
byte brightness = 240;
const byte maxBrightness = 245;
const byte brightnessOffset = 10;

const byte lcdWidth = 16;
const byte lcdHeight = 2;

const byte minContrast = 20;
byte contrast = 110;
const byte maxContrast = 119;

int level = minLevel;

String playerName = defaultName;
byte playerNameIndex = 0;
const byte startPlayerHighscore = 255;
byte playerHighscore = startPlayerHighscore;
const byte defaultPlayerLife = 5;
byte playerLife = defaultPlayerLife;

const byte contrastAddress = 0;
const byte brightnessAddress = 1;
const byte matrixBrightnessAddress = 2;
const byte highscoreAddress = 3;
const byte soundAddress = 4;

const byte mapHeight = 8;
const byte mapWidth = 64;

unsigned long long gameMap[mapHeight];
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

const byte randomPin = 0;

const byte maxEnemiesCount = 15;
byte enemyRows[maxEnemiesCount];
byte enemyCols[maxEnemiesCount];

enum enemyDirection {
  leftDirection,
  rightDirection
};

enemyDirection enemyDirections[maxEnemiesCount];
byte enemiesCount;

const byte minDistanceBetweenEnemies = 3;

const byte shootingRange = 5;

const byte nonExistantBullet = -1;

const byte defaultEnemyLife = 1;

const int scrollingFrequency = 25;
const int jumpFrequency = 800;
const int shootingFrequency = 500;
const int menuClickFrequency = 100;
const int menuSelectFrequency = 300;

int playerBulletRow = nonExistantBullet;
int playerBulletCol = nonExistantBullet;

int enemyBulletRows[maxEnemiesCount];
int enemyBulletCols[maxEnemiesCount];
enemyDirection enemyBulletDirection[maxEnemiesCount];
byte enemyLife[maxEnemiesCount];

bool sound = true;

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
  setMatrixBrightness,
  deathScreen,
  winScreen,
  resetHighscores,
  setSound
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

void toneBuzzer(int frequency) {

  const byte defaultDuration = 255;

  if (sound == false) {
    noTone(buzzerPin);
  }
  else {
    tone(buzzerPin, frequency, defaultDuration);
  }
}

void changeJoystickSwState() {

  static const byte debounceInterval = 200;
  static unsigned long long lastInterruptTime = 0;

  unsigned long long interruptTime = millis();

  if (interruptTime - lastInterruptTime >= debounceInterval) {
    joystickSwState = !joystickSwState;
  }

  lastInterruptTime = interruptTime;
}

bool getCell(int row, int col) {

  return gameMap[row] >> (mapWidth - col - 1) & 1;
}

void setCell(int row, int col, bool val) { // val = 0 or 1

  gameMap[row] ^= (-val ^ gameMap[row]) & (1ULL << (mapWidth - col - 1));
}

void setup() {

  pinMode(joystickPinSW, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(joystickPinSW), changeJoystickSwState, FALLING); // set interrupt for joystick button

  pinMode(joystickPinX, INPUT);
  pinMode(joystickPinY, INPUT);

  pinMode(contrastPin, OUTPUT);

  pinMode(brightnessPin, OUTPUT);

  contrast = EEPROM.read(contrastAddress);
  brightness = EEPROM.read(brightnessAddress);
  matrixBrightness = EEPROM.read(matrixBrightnessAddress);
  sound = EEPROM.read(soundAddress);

  lc.shutdown(0, false);
  lc.setIntensity(0, matrixBrightness);
  lc.clearDisplay(0);

  gameState = welcomeScreen;
  lcd.begin(lcdWidth, lcdHeight);
  lcd.createChar(downScrollingArrowLcdId, downScrollingArrow);
  lcd.createChar(upScrollingArrowLcdId, upScrollingArrow);
  lcd.createChar(selectionArrowLcdId, selectionArrow);
  lcd.createChar(increaseArrowLcdId, increaseArrow);
  lcd.createChar(decreaseArrowLcdId, decreaseArrow);

  // load the highscores from EEPROM
  for (int i = 0, address = highscoreAddress; i < maxHighscoresCount; i++, address++) {
    String copyName = "";
    for (int characterIndex = 0; characterIndex < nameSize; characterIndex++) {
      copyName += char(EEPROM.read(address));
      address++;
    }
    highscoreNames[i] = copyName;
    highscores[i] = EEPROM.read(address);
  }

  randomSeed(analogRead(randomPin));

  analogWrite(contrastPin, contrast);
  analogWrite(brightnessPin, brightness);
}

// setup the platforms for a new level
void generateLevel() {

  byte lastPlatformHeight = 3;
  byte lastPlatformEnd = -1;

  for (int col = 0; col < mapWidth;) {
    bool startPlatform;

    if (col - lastPlatformEnd > jumpSize) { // avoid having unreachable platforms
      startPlatform = true;
    }
    else {
      startPlatform = random(2); // 0 or 1
    }

    if (startPlatform) {
      byte platformLength = random(minPlatformLength, min(mapWidth - col, maxPlatformLength));
      byte mapRow = random(max(lastPlatformHeight - jumpSize, characterSize + 1), mapHeight); // the row on the matrix where the next platform will be spawned
      for (int i = col; i < col + platformLength; i++) {
        setCell(mapRow, i, true);
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

// displays all elements of the game on the matrix; instead of just using (row, col) I mirrored the positions because of the physical orientation of the matrix
// objects are rendered only if they are in the range of the camera
void displayMap() {

  lc.clearDisplay(0);
  for (int row = 0; row < matrixSize; row++) { // display the platforms
    for (int col = cameraLeftPosition; col < cameraRightPosition; col++) {
      lc.setLed(0, matrixSize - (col - cameraLeftPosition) - 1, row, getCell(row, col));
    }
  }

  for (int row = playerRow; row > playerRow - characterSize; row--) { // display the player
    lc.setLed(0, matrixSize - (playerCol - cameraLeftPosition) - 1, row, true);
  }

  lc.setLed(0, matrixSize - (playerCol + 1 - cameraLeftPosition) - 1, playerRow - 1, true); // display hand

  for (int i = 0; i < enemiesCount; i++) { // render enemies
    if (enemyCols[i] >= cameraLeftPosition && enemyCols[i] < cameraRightPosition) {
      for (int row = enemyRows[i]; row > enemyRows[i] - characterSize; row--) {
        lc.setLed(0, matrixSize - (enemyCols[i] - cameraLeftPosition) - 1, row, true);
      }

      if (enemyDirections[i] == leftDirection) {
        lc.setLed(0, matrixSize - (enemyCols[i] - 1 - cameraLeftPosition) - 1, enemyRows[i] - 1, true);
      }
      else {
        lc.setLed(0, matrixSize - (enemyCols[i] + 1 - cameraLeftPosition) - 1, enemyRows[i] - 1, true);
      }
    }
  }

  if (playerBulletCol >= cameraLeftPosition && playerBulletCol < cameraRightPosition) { // player bullet
    lc.setLed(0, matrixSize - (playerBulletCol - cameraLeftPosition) - 1, playerBulletRow, true);
  }

  for (int i = 0; i < enemiesCount; i++) { // enemy bullets
    if (enemyBulletCols[i] >= cameraLeftPosition && enemyBulletCols[i] < cameraRightPosition) {
      lc.setLed(0, matrixSize - (enemyBulletCols[i] - cameraLeftPosition) - 1, enemyBulletRows[i], true);
    }
  }
}

void setStartingPosition() {

  for (int col = 0; col < mapWidth; col++) {
    for (int row = 0; row < mapHeight; row++) {
      if (getCell(row, col)) {
        playerRow = row - 1;
        playerCol = col;

        return ;
      }
    }
  }
}

// checks if the next position will be a wall
bool checkCollision(int nextRow, int nextCol, bool isEnemy = false) {

  if (nextRow >= matrixSize || nextRow < 0 || nextCol < 0 || nextCol >= mapWidth) {
    return false;
  }

  for (int row = nextRow; row > playerRow - characterSize; row--) {
    if (getCell(row, nextCol)) {
      return true;
    }
  }

  if (isEnemy) {
    if (getCell(nextRow - 1, nextCol - 1)) {
      return true;
    }
  }
  else {
    if (getCell(nextRow - 1, nextCol + 1)) {
      return true;
    }
  }

  return false;
}

float genFloatRandom(int left, int right, int decimalCount) { // left and right of interval

  int powerOfTen = 1;
  while (decimalCount - 1) {
    powerOfTen *= 10;
    decimalCount--;
  }

  return random(left * powerOfTen, right * powerOfTen) / (1.0 * powerOfTen);
}

void generateEnemies() {

  const float minProbability = 0.0f;
  const float maxProbability = 1.0f;
  const byte decimalCount = 2;
  float enemySpawningProbability = maxProbability - (1 + maxLevel - level) / 10.0f;

  for (int row = 0; row < mapHeight; row++) {
    for (int col = mapWidth - 1; col >= mapWidth / 4; col--) { // go from right to left on the map and stop once you reach the first quarter of the map, this way I avoid spawning enemies right to the player
      if (getCell(row, col) && enemiesCount < maxEnemiesCount) { // if there is a platform at this position and I can still spawn enemies
        if (enemiesCount == 0 || (enemiesCount > 0 && enemyCols[enemiesCount - 1] - col >= minDistanceBetweenEnemies)) {
          float spawnEnemy = genFloatRandom(minProbability, maxProbability, decimalCount);

          if (spawnEnemy > maxProbability - enemySpawningProbability) {
            enemyRows[enemiesCount] = row - 1;
            enemyCols[enemiesCount] = col;
            enemyDirections[enemiesCount] = random(2) == 0 ? leftDirection : rightDirection;
            enemiesCount++;
          }
        }
      }
    }
  }

  for (int i = 0; i < enemiesCount; i++) {
    enemyLife[i] = level > maxLevel / 2 ? defaultEnemyLife + 1 : defaultEnemyLife; // set enemy life to 2 for hard and insane and 1 otherwise
    enemyBulletRows[i] = nonExistantBullet;
  }
}

bool playerInShootingRange(int row, int col, enemyDirection dir) {

  if (abs(col - playerCol) <= shootingRange) {
    for (int pRow = playerRow; pRow > playerRow - characterSize; pRow--) {
      if (pRow == row - 1) {
        return true;
      }
    }
  }

  return false;
}

void updateEnemiesPositions() { // move enemies left and right

  static unsigned long long lastUpdate = 0;
  const int platformMovementInterval = 500;

  if (millis() - lastUpdate >= platformMovementInterval) {
    lastUpdate = millis();
    for (int i = 0; i < enemiesCount; i++) {
      if (playerInShootingRange(enemyRows[i], enemyCols[i], enemyDirections[i])) { // if player is in shooting range change the direction of the enemy towards the player
        if (playerCol < enemyCols[i]) {
          enemyDirections[i] = leftDirection;
        }
        else {
          enemyDirections[i] = rightDirection;
        }
      }
      else { // go left and right
        if (getCell(enemyRows[i] + 1, enemyCols[i] - (enemyDirections[i] == leftDirection ? 1 : -1)) == 0) {
          enemyDirections[i] = enemyDirections[i] == leftDirection ? rightDirection : leftDirection;
        }
        else {
          if (enemyDirections[i] == leftDirection) {
            enemyCols[i]--;
          }
          else {
            enemyCols[i]++;
          }
        }
      }
    }
  }
}

int enemyCollision(int row, int col) { // check if enemy hits a platform

  for (int i = 0; i < enemiesCount; i++) {
    for (int eRow = enemyRows[i]; eRow > enemyRows[i] - characterSize; eRow--) {
      if (eRow == row && enemyCols[i] == col) {
        return i;
      }
    }
    if (row == enemyRows[i] - 1 && col == enemyCols + (enemyDirections[i] == leftDirection ? -1 : 1)) {
      return i;
    }
  }

  return -1;
}

void updatePlayerBullet() {

  static unsigned long long lastUpdate = 0;
  const int updateInterval = 100;
  static byte distanceTravelled = 0;

  if (joystickSwState != lastJoystickSwState && playerBulletRow == nonExistantBullet) { // shoot if joystick is pressed
    playerBulletRow = playerRow - 1;
    playerBulletCol = playerCol + 1;
    distanceTravelled = 0;
    toneBuzzer(shootingFrequency);
  }

  if (playerBulletRow != nonExistantBullet) {
    if (distanceTravelled >= shootingRange || checkCollision(playerBulletRow, playerBulletCol + 1)) { // erase the player bullet after some distance or if it hit a wall
      playerBulletRow = nonExistantBullet;
      playerBulletCol = nonExistantBullet;
    }

    int hitEnemy = enemyCollision(playerBulletRow, playerBulletCol + 1);
    if (hitEnemy != -1) {
      playerBulletRow = nonExistantBullet;
      playerBulletCol = nonExistantBullet;
      enemyLife[hitEnemy]--;
    }

    if (millis() - lastUpdate >= updateInterval) {
      lastUpdate = millis();
      playerBulletCol++;
      distanceTravelled++;
    }
  }
}

void updateDeadEnemies() {

  for (int i = enemiesCount - 1; i >= 0; i--) {
    if (enemyLife[i] == 0) {
      for (int j = i; j < enemiesCount; j++) { // override current enemy
        enemyLife[j] = enemyLife[j + 1];
        enemyRows[j] = enemyRows[j + 1];
        enemyCols[j] = enemyCols[j + 1];
        enemyDirections[j] = enemyDirections[j + 1];
      }
      enemiesCount--;
    }
  }
}

bool playerCollision(int row, int col) { // check if player hits a wall

  for (int pRow = playerRow; pRow > playerRow - characterSize; --pRow) {
    if (row == pRow && col == playerCol) {
      return true;
    }
  }

  if (row == playerRow - 1 && col == playerCol + 1) {
    return true;
  }

  return false;
}

void updateEnemyBullets() {

  static unsigned long long lastBulletUpdate[maxEnemiesCount];
  static byte enemyBulletTravelledDistance[maxEnemiesCount];
  const int defaultBulletUpdateInterval = 600;
  const byte bulletUpdateIntervalOffset = 50;
  int bulletUpdateInterval = defaultBulletUpdateInterval - bulletUpdateIntervalOffset * (level - 1);

  for (int i = enemiesCount - 1; i >= 0; i--) {
    if (enemyBulletRows[i] == nonExistantBullet) { // no bullet exists
      if (playerInShootingRange(enemyRows[i], enemyCols[i], enemyDirections[i])) { // if player is in shooting range make enemy shoot
        enemyBulletRows[i] = enemyRows[i] - 1;
        enemyBulletCols[i] = enemyCols[i] + (enemyDirections[i] == leftDirection ? -2 : 2);
        enemyBulletDirection[i] = enemyDirections[i];
        enemyBulletTravelledDistance[i] = 0;
      }
    }
    else {
      if (millis() - lastBulletUpdate[i] >= bulletUpdateInterval) { // move the bullet
        lastBulletUpdate[i] = millis();
        enemyBulletCols[i] += (enemyBulletDirection[i] == leftDirection ? -1 : 1);
        enemyBulletTravelledDistance[i]++;
        fovUpdate = true;
      }
      if (playerCollision(enemyBulletRows[i], enemyBulletCols[i])) { // if the player is hit
        playerLife--;
        enemyBulletRows[i] = enemyBulletCols[i] = nonExistantBullet;
      }
      else if (enemyBulletTravelledDistance[i] >= shootingRange || checkCollision(enemyBulletRows[i], enemyBulletCols[i] + (enemyBulletDirection[i] == leftDirection ? -1 : 1))) { // check if exceed the shooting range or if the bullet hit a wall
        enemyBulletRows[i] = enemyBulletCols[i] = nonExistantBullet;
      }
    }
  }
}

void displayInGameStats() {

  static byte lastLife = 0;
  static byte lastEnemiesCount = 0;
  static byte lastHighscore = 0;

  if (lastLife != playerLife || lastEnemiesCount != enemiesCount || lastHighscore != playerHighscore) { // if some stat updated refresh the lcd
    lcd.clear();
    lcd.home();
    lcd.print("Life:" + String(playerLife) + " Score:" + String(playerHighscore));
    lcd.setCursor(0, lcdHeight - 1);
    lcd.print("Enemies:" + String(enemiesCount));
  }

  lastLife = playerLife;
  lastEnemiesCount = enemiesCount;
  lastHighscore = playerHighscore;
}

void updateHighscore() {

  static unsigned long long lastUpdate = 0;
  const unsigned long long updateInterval = 500;

  if (millis() - lastUpdate > updateInterval) {
    lastUpdate = millis();
    if (playerHighscore - (1 + maxLevel - level) > 0) {
      playerHighscore -= (1 + maxLevel - level); // lower the score as the game progresses depending on the difficulty (faster for easy and slower for insane)
    }
    else {
      gameState = deathScreen;
    }
  }
}

void runPlayGame() {

  static bool startOfLevel = true;
  static unsigned long long lastJump = 0;
  const byte jumpInterval = 200;
  const byte fallInterval = 200;
  static unsigned long long lastFall = 0;

  if (startOfLevel) { // initialize level
    for (int row = 0; row < mapHeight; row++) {
      for (int col = 0; col < mapWidth; col++) {
        setCell(row, col, false);
      }
    }
    playerLife = defaultPlayerLife;
    enemiesCount = 0;
    cameraLeftPosition = 0;
    cameraRightPosition = matrixSize;
    playerHighscore = startPlayerHighscore;
    generateLevel();
    setStartingPosition();
    generateEnemies();
    startOfLevel = false;
    fovUpdate = true;
  }

  displayInGameStats();
  updateHighscore();

  updateEnemiesPositions();
  updateEnemyBullets();

  if (jumpsLeft) {
    if (playerRow - characterSize + 1 > 0) {
      if (millis() - lastJump >= jumpInterval) {
        jumpsLeft--;
        playerRow--;
        lastJump = millis();
      }
    }
    else { // it hit the ceiling of the map
      jumpsLeft = 0;
    }
  }

  int joystickMove = joystickVerticalMove();

  if (joystickMove == up && (checkCollision(playerRow + 1, playerCol) || checkCollision(playerRow, playerCol + 1))) { // player jumps
    jumpsLeft = jumpSize;
    toneBuzzer(jumpFrequency);
  }

  if (playerRow == mapHeight || playerLife == 0 || enemiesCount == 0) { // end of game if player fell off a platform or died or all enemies are dead
    startOfLevel = true;
    if (enemiesCount == 0) {
      gameState = winScreen;
    }
    else {
      gameState = deathScreen;
    }
    return ;
  }

  if (!checkCollision(playerRow + 1, playerCol) && jumpsLeft == 0 && millis() - lastFall > fallInterval) { // go down if there is no platform
    playerRow++;
    lastFall = millis();
    fovUpdate = true;
  }

  joystickMove = joystickHorizontalMove();

  if (joystickMove == left && cameraLeftPosition > 0 && !checkCollision(playerRow, playerCol - 1)) { // player moves left
    if (mapWidth - matrixSize + 1 < playerCol) {
      playerCol--;
    }
    else {
      cameraLeftPosition--;
      cameraRightPosition--;
      playerCol--;
    }
    fovUpdate = true;
  }

  if (joystickMove == right && !checkCollision(playerRow, playerCol + 1)) { // player moves right
    if (cameraRightPosition < mapWidth) {
      cameraLeftPosition++;
      cameraRightPosition++;
      playerCol++;
      fovUpdate = true;
    }
    else if (playerCol < mapWidth - 1) {
      playerCol++;
      fovUpdate = true;
    }
  }

  if (fovUpdate) { // refresh the matrix if there are any updates on the field of view
    displayMap();
  }

  updateDeadEnemies();
  updatePlayerBullet();
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
  else if (gameState == deathScreen) {
    runDeathScreen();
  }
  else if (gameState == winScreen) {
    runWinScreen();
  }
  else if (gameState == resetHighscores) {
    runResetHighscores();
  }
  else if (gameState == setSound) {
    runSetSound();
  }

  lastJoystickSwState = joystickSwState;
}

bool playerObtainedHighscore() { // this checks wether the player obtained a new highscore or not

  for (int i = 0; i < maxHighscoresCount; i++) {
    if (highscores[i] < playerHighscore) {
      return true;
    }
  }
  return false;
}

void runWinScreen() {

  static bool displayedWin = false;
  static bool displayedNewHighscore = false;
  const int winScreenDuration = 2000;
  static unsigned long long winScreenStart = 0;

  if (winScreenStart == 0) {
    winScreenStart = millis();
  }

  // first display the congrats message then the new highscore message
  if (millis() - winScreenStart < winScreenDuration) {
    if (!displayedWin) {
      displayedWin = true;
      lcd.clear();
      lcd.home();
      lcd.print("You won!");
      lcd.setCursor(0, lcdHeight - 1);
      lcd.print("Your score:" + String(playerHighscore));
    }
  }
  else if (millis() - winScreenStart < 2 * winScreenDuration) {
    if (playerObtainedHighscore()) {
      if (!displayedNewHighscore) {
        displayedNewHighscore = true;
        lcd.clear();
        lcd.home();
        lcd.print("You obtained");
        lcd.setCursor(0, lcdHeight - 1);
        lcd.print("a new highscore!");
      }
    }
  }
  else {
    displayedWin = false;
    displayedNewHighscore = false;
    winScreenStart = 0;
    gameState = changeName;
  }
}

void runDeathScreen() {

  const int deathScreenDuration = 2000;
  static unsigned long long deathScreenStart = 0;
  static bool displayed = false;

  if (deathScreenStart == 0) {
    deathScreenStart = millis();
  }

  if (millis() - deathScreenStart < deathScreenDuration) {
    if (!displayed) {
      lcd.clear();
      lcd.home(); // first row
      lcd.print("Oh no, you died!");
      lcd.setCursor(0, lcdHeight - 1);
      lcd.print("Your score: " + String(playerHighscore));
      displayed = true;
    }
  }
  else {
    lcd.clear();
    deathScreenStart = 0;
    displayed = false;
    gameState = homeScreen;
  }
}

void runWelcomeScreen() {

  const int welcomeScreenDuration = 2000;
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

joystickMove joystickVerticalMove() { // up and down joystick movement

  static bool joystickMoved = false;
  const int samplesCount = 300;

  float average = 0.0f;

  for (int i = 0; i < samplesCount; i++) { // average multiple values because of the noise
    int xValue = analogRead(joystickPinX);
    average += xValue;
  }

  int xValue = average / samplesCount;

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


int renderScrollingMenu(String contents[], int contentsLength, bool useSelection = false) { // renders some scrolling texts on the lcd

  static byte menuRow = 0;
  static byte lastMenuRow = 1;
  static byte selectedRow = 0;
  static byte lastSelectedRow = 0;

  int joystickMove = joystickVerticalMove();

  // movement through the menu (up and down)
  if (joystickMove == up) {
    if (menuRow) {
      menuRow--;
      toneBuzzer(scrollingFrequency);
    }
    if (selectedRow) {
      selectedRow--;
      toneBuzzer(scrollingFrequency);
    }
  }
  else if (joystickMove == down) {
    if (menuRow < contentsLength - lcdHeight) {
      menuRow++;
      toneBuzzer(scrollingFrequency);
    }
    if (selectedRow < contentsLength - 1) {
      selectedRow++;
      toneBuzzer(scrollingFrequency);
    }

  }

  if (menuRow != lastMenuRow || selectedRow != lastSelectedRow) { // refresh if anything changes

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
    toneBuzzer(menuClickFrequency);

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

  const byte optionsLength = 4;
  const String options[] = {
    "1.Play",
    "2.Settings",
    "3.Highscores",
    "4.About",
  };

  int exitCode = renderScrollingMenu(options, optionsLength, true);

  // the exit codes correspond (1 through 4) to the options of the menu (see the variable options[])
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
    else if (exitCode == 4) {
      gameState = about;
    }
  }
}

void runAboutMenu() {

  const int aboutDescriptionRowsCount = 8;
  const String aboutDescription[aboutDescriptionRowsCount] = {
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

  const int settingsCount = 7;
  String settings[settingsCount] = {
    "1.Level",
    "2.Contrast",
    "3.Brightness",
    "4.Game light",
    "5.Reset highs",
    "6.Sound",
    "7.Back"
  };

  int exitCode = renderScrollingMenu(settings, settingsCount, true);

  // the exit codes correspond (1 through 7) to the options of the menu (see the variable settings[])

  if (exitCode != -1) {
    exitCode++;
  }

  if (exitCode == 7) {
    gameState = homeScreen;
  }
  else if (exitCode == 1) {
    gameState = setLevel;
  }
  else if (exitCode == 2) {
    gameState = setContrast;
  }
  else if (exitCode == 3) {
    gameState = setBrightness;
  }
  else if (exitCode == 4) {
    gameState = setMatrixBrightness;
  }
  else if (exitCode == 5) {
    gameState = resetHighscores;
  }
  else if (exitCode == 6) {
    gameState = setSound;
  }
}

void resetHighscoresEEPROM() {

  const int defaultHighscore = 1;

  for (int i = 0; i < maxHighscoresCount; i++) {
    highscoreNames[i] = defaultName;
    highscores[i] = defaultHighscore;
  }

  for (int i = 0, address = highscoreAddress; i < maxHighscoresCount; i++, address++) {
    for (int characterIndex = 0; characterIndex < nameSize; characterIndex++) {
      EEPROM.update(address, highscoreNames[i][characterIndex]);
      address++;
    }
    EEPROM.update(address, highscores[i]);
  }
}

void runResetHighscores() {

  static bool reseted = false;
  static unsigned long long messageStart = 0;
  const unsigned int messageDuration = 2000;
  static bool messageDisplayed = false;

  if (!reseted) {
    reseted = true;
    resetHighscoresEEPROM();
    messageStart = millis();
  }
  else if (millis() - messageStart <= messageDuration) {
    if (!messageDisplayed) {
      lcd.clear();
      messageDisplayed = true;
      lcd.home();
      lcd.print("Highscores have");
      lcd.setCursor(0, lcdHeight - 1);
      lcd.print("been reseted");
    }
  }
  else {
    gameState = settings;
    reseted = false;
    messageDisplayed = false;
  }
}

void runSetSound() {

  static bool soundChanged = false;
  static int firstRun = -1;

  int joystickMove = joystickHorizontalMove();

  if (joystickMove == left) {
    soundChanged = true;
    sound = false;
    toneBuzzer(menuSelectFrequency);
  }

  if (joystickMove == right) {
    soundChanged = true;
    sound = true;
    toneBuzzer(menuSelectFrequency);
  }

  if (joystickMove == neutral) {
    soundChanged = false;
  }

  if (soundChanged || firstRun == -1) {
    lcd.clear();

    lcd.home();
    lcd.print("Turn ON/OFF:");

    int messageLength = sound ? 2 : 3; // ON is 2 chars and OFF is 3 chars
    int startPosition = (lcdWidth - 4 - messageLength) / 2;

    lcd.setCursor(startPosition, lcdHeight - 1);
    lcd.write(byte(decreaseArrowLcdId));

    lcd.setCursor(startPosition + 2, lcdHeight - 1);
    lcd.print(sound == false ? "OFF" : "ON");

    lcd.setCursor(startPosition + 3 + messageLength, lcdHeight - 1);
    lcd.write(byte(increaseArrowLcdId));
  }

  if (firstRun == -1) {
    firstRun = 0;
  }

  if (joystickSwState != lastJoystickSwState) {
    gameState = settings;
    firstRun = -1;
    toneBuzzer(menuClickFrequency);
    EEPROM.update(soundAddress, sound);
  }
}

int digitsCount(int no) {

  int count = 1;

  while (no > 9) { // go until there is only one digit left (this is good in case the input is 0)
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

  if (exitCode == -1) {
    lcdContents[0] = "";
  }
}

joystickMove joystickHorizontalMove() {

  static unsigned long long lastChange = 0;
  static byte updateInterval = 200;
  const int samplesCount = 300;

  if (millis() - lastChange > updateInterval) {
    float average = 0.0f;

    for (int i = 0; i < samplesCount; i++) {
      int yValue = analogRead(joystickPinY);
      average += yValue;
    }

    int yValue = average / samplesCount;

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

  static int lastPlayerNameIndex = -1;
  static bool changedLetter = false;

  int joystickMoveHorizontal = joystickHorizontalMove();

  if (joystickMoveHorizontal == left && playerNameIndex > 0) {
    playerNameIndex--;
    toneBuzzer(menuSelectFrequency);
  }

  if (joystickMoveHorizontal == right && playerNameIndex < nameSize - 1) {
    playerNameIndex++;
    toneBuzzer(menuSelectFrequency);
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
    lastPlayerNameIndex = -1;
    changedLetter = false;
    updateHighscores();
    gameState = homeScreen;
    toneBuzzer(menuClickFrequency);
    lcd.noCursor();
  }
}

void runSetLevel() {

  static int lastLevel = 0;
  const String difficulty[maxLevel] = {
    "Easy",
    "Medium",
    "Hard",
    "Insane"
  };

  int joystickMove = joystickHorizontalMove();

  if (joystickMove == left && level > minLevel) {
    level--;
    toneBuzzer(menuSelectFrequency);
  }

  if (joystickMove == right && level < maxLevel) {
    level++;
    toneBuzzer(menuSelectFrequency);
  }

  String title = "Select level:";

  if (level != lastLevel) {
    lcd.clear();

    lcd.home();
    lcd.print(title);

    byte startPosition = (lcdWidth - difficulty[level - 1].length() - 4) / 2;

    lcd.setCursor(startPosition, lcdHeight - 1);
    lcd.write(byte(decreaseArrowLcdId));

    lcd.setCursor(startPosition + 2, lcdHeight - 1);
    lcd.print(difficulty[level - 1]);

    lcd.setCursor(startPosition + 3 + difficulty[level - 1].length(), lcdHeight - 1);
    lcd.write(byte(increaseArrowLcdId));
  }

  lastLevel = level;

  if (joystickSwState != lastJoystickSwState) {
    lastLevel = 0;
    gameState = settings;
    toneBuzzer(menuClickFrequency);
  }
}

void runSetContrast() {

  static int lastContrast = 0;

  int joystickMove = joystickHorizontalMove();

  if (joystickMove == left && contrast > minContrast) {
    contrast--;
    toneBuzzer(menuSelectFrequency);
  }

  if (joystickMove == right && contrast < maxContrast) {
    contrast++;
    toneBuzzer(menuSelectFrequency);
  }

  String title = "Select contrast:";

  if (contrast != lastContrast) {
    lcd.clear();

    lcd.home();
    lcd.print(title);

    lcd.setCursor(lcdWidth / 2 - 2, lcdHeight - 1);
    lcd.write(byte(decreaseArrowLcdId));

    lcd.setCursor(lcdWidth / 2 + 2, lcdHeight - 1);
    lcd.write(byte(increaseArrowLcdId));

    lcd.setCursor(lcdWidth / 2, lcdHeight - 1);
    lcd.print(contrast - minContrast);

    analogWrite(contrastPin, contrast);
  }

  lastContrast = contrast;

  if (joystickSwState != lastJoystickSwState) {
    lastContrast = 0;
    EEPROM.update(contrastAddress, contrast);
    gameState = settings;
    toneBuzzer(menuClickFrequency);
  }
}

void runSetBrightness() {

  static int lastBrightness = 0;

  int joystickMove = joystickHorizontalMove();

  if (joystickMove == left && brightness > minBrightness) {
    brightness -= brightnessOffset;
    toneBuzzer(menuSelectFrequency);
  }

  if (joystickMove == right && brightness < maxBrightness) {
    brightness += brightnessOffset;
    toneBuzzer(menuSelectFrequency);
  }

  String title = "Brightness:";

  if (brightness != lastBrightness) {
    lcd.clear();

    lcd.home();
    lcd.print(title);

    int startPosition = lcdWidth - 4 - digitsCount(brightness - minBrightness);
    startPosition /= 2;

    lcd.setCursor(startPosition, lcdHeight - 1);
    lcd.write(byte(decreaseArrowLcdId));

    lcd.setCursor(startPosition + digitsCount(brightness - minBrightness) + 3, lcdHeight - 1);
    lcd.write(byte(increaseArrowLcdId));

    lcd.setCursor(startPosition + 2, lcdHeight - 1);
    lcd.print(brightness - minBrightness);

    analogWrite(brightnessPin, brightness);
  }

  lastBrightness = brightness;

  if (joystickSwState != lastJoystickSwState) {
    lastBrightness = 0;
    EEPROM.update(brightnessAddress, brightness);
    gameState = settings;
    toneBuzzer(menuClickFrequency);
  }
}

void runSetMatrixBrightness() {

  static int lastMatrixBrightness = 0;

  int joystickMove = joystickHorizontalMove();

  if (joystickMove == left && matrixBrightness > minMatrixBrightness) {
    matrixBrightness--;
    toneBuzzer(menuSelectFrequency);
  }

  if (joystickMove == right && matrixBrightness < maxMatrixBrightness) {
    matrixBrightness++;
    toneBuzzer(menuSelectFrequency);
  }

  String title = "Game brightness:";

  if (matrixBrightness != lastMatrixBrightness) {
    lcd.clear();

    lcd.home();
    lcd.print(title);

    lcd.setCursor(lcdWidth / 2 - 2, lcdHeight - 1);
    lcd.write(byte(decreaseArrowLcdId));

    lcd.setCursor(lcdWidth / 2 + 2, lcdHeight - 1);
    lcd.write(byte(increaseArrowLcdId));

    lcd.setCursor(lcdWidth / 2, lcdHeight - 1);
    lcd.print(matrixBrightness);

    lc.setIntensity(0, matrixBrightness);
    lc.clearDisplay(0);

    for (int row = 0; row < matrixSize; row++) {
      for (int col = 0; col < matrixSize; col++) {
        lc.setLed(0, row, col, true);
      }
    }
  }

  lastMatrixBrightness = matrixBrightness;

  if (joystickSwState != lastJoystickSwState) {
    lastMatrixBrightness = 0;
    EEPROM.update(matrixBrightnessAddress, matrixBrightness);
    gameState = settings;
    toneBuzzer(menuClickFrequency);
  }
}

void updateHighscores() {

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

  // save to EEPROM

  for (int i = 0, address = highscoreAddress; i < maxHighscoresCount; i++, address++) {
    for (int characterIndex = 0; characterIndex < nameSize; characterIndex++) {
      EEPROM.update(address, highscoreNames[i][characterIndex]);
      address++;
    }
    EEPROM.update(address, highscores[i]);
  }
}
