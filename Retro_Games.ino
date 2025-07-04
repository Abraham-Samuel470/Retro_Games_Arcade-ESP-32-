// === ESP32 Retro Game Console with 6 Games ===
// Games: Dino Runner, Space Shooter, Flappy Bird, Pong, Maze Escape, Brick Breaker

#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET    -1
#define SCREEN_ADDRESS 0x3C

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

#define BTN_LEFT   33
#define BTN_RIGHT  25
#define BTN_SELECT 32
#define BUZZER     26

String gameList[] = {"Dino", "Space", "Flappy", "Pong", "Maze", "Brick"};
int selected = 0;

enum GameState { MENU, GAME_DINO, GAME_SPACE, GAME_FLAPPY, GAME_PONG, GAME_MAZE, GAME_BRICK };
GameState currentGame = MENU;

// === Dino Game Variables ===
int dinoY, jumpVel, cactusX, dinoScore;
bool jumping;

// === Space Shooter ===
int playerX, bulletX, bulletY, enemyX, enemyY, spaceScore;
bool bulletActive, spaceGameOver;
const int playerY = 58;

// === Flappy Bird ===
int birdY, birdVel, pipeX, gapY, flappyScore;
bool flappyGameOver;

// === Pong ===
int paddle1Y = 24, paddle2Y = 24, ballX = 64, ballY = 32, ballVelX = 2, ballVelY = 2;
int pongScore1 = 0, pongScore2 = 0;
bool singlePlayer = true;

// === Maze ===
int mazeX = 0, mazeY = 0;
int goalX = 120, goalY = 56;

// === Brick Breaker ===
int paddleX = 50, bX = 64, bY = 32, bVelX = 2, bVelY = -2;
bool bricks[5][8];
int brickScore = 0;

void setup() {
  Serial.begin(115200);
  Wire.begin(21, 22);
  pinMode(BTN_LEFT, INPUT_PULLUP);
  pinMode(BTN_RIGHT, INPUT_PULLUP);
  pinMode(BTN_SELECT, INPUT_PULLUP);
  pinMode(BUZZER, OUTPUT);

  if (!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    Serial.println(F("SSD1306 allocation failed"));
    while (true);
  }

  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(20, 25);
  display.println("Retro Game Console");
  display.display();
  delay(2000);
}

void loop() {
  switch (currentGame) {
    case MENU: runMenu(); break;
    case GAME_DINO: runDino(); break;
    case GAME_SPACE: runSpace(); break;
    case GAME_FLAPPY: runFlappy(); break;
    case GAME_PONG: runPong(); break;
    case GAME_MAZE: runMaze(); break;
    case GAME_BRICK: runBrick(); break;
  }
}

// === MENU ===
void runMenu() {
  display.clearDisplay();
  display.setCursor(0, 0);
  display.println("== Select Game ==");
  for (int i = 0; i < 6; i++) {
    if (i == selected) display.print("> ");
    else display.print("  ");
    display.println(gameList[i]);
  }
  display.display();

  if (digitalRead(BTN_RIGHT) == LOW) { selected = (selected + 1) % 6; tone(BUZZER, 1000, 100); delay(200); }
  if (digitalRead(BTN_LEFT) == LOW) { selected = (selected - 1 + 6) % 6; tone(BUZZER, 800, 100); delay(200); }
  if (digitalRead(BTN_SELECT) == LOW) {
    tone(BUZZER, 2000, 100); delay(300);
    currentGame = static_cast<GameState>(selected + 1);
    initGame();
  }
}

// === INIT GAMES ===
void initGame() {
  dinoY = 48; jumping = false; cactusX = SCREEN_WIDTH; dinoScore = 0;
  playerX = 64; bulletActive = false; enemyX = random(10, SCREEN_WIDTH - 10); enemyY = 0; spaceScore = 0; spaceGameOver = false;
  birdY = 32; birdVel = 0; pipeX = SCREEN_WIDTH; gapY = random(20, 40); flappyScore = 0; flappyGameOver = false;
  paddle1Y = 24; paddle2Y = 24; ballX = 64; ballY = 32; ballVelX = 2; ballVelY = 2;
  mazeX = 0; mazeY = 0;
  paddleX = 50; bX = 64; bY = 32; bVelX = 2; bVelY = -2; brickScore = 0;
  for (int i = 0; i < 5; i++) for (int j = 0; j < 8; j++) bricks[i][j] = true;
}

int gameOverOption = 0; // 0 - Play Again, 1 - Exit

void gameOverScreen(String gameName, int score) {
  while (true) {
    display.clearDisplay();
    display.setCursor(10, 10);
    display.println(gameName + " Game Over");
    display.setCursor(10, 25);
    display.print("Score: ");
    display.println(score);
    
    display.setCursor(10, 45);
    display.print(gameOverOption == 0 ? "> Play Again" : "  Play Again");
    display.setCursor(10, 55);
    display.print(gameOverOption == 1 ? "> Exit" : "  Exit");
    display.display();

    if (digitalRead(BTN_LEFT) == LOW || digitalRead(BTN_RIGHT) == LOW) {
      gameOverOption = 1 - gameOverOption;
      tone(BUZZER, 1000, 100);
      delay(200);
    }

    if (digitalRead(BTN_SELECT) == LOW) {
      tone(BUZZER, 1500, 100);
      delay(200);
      if (gameOverOption == 0) {
        initGame();
        return; // Continue current game
      } else {
        currentGame = MENU;
        return;
      }
    }
  }
}


// === BEEP ===
void beep(int freq, int dur) { tone(BUZZER, freq, dur); delay(dur); noTone(BUZZER); }
void tripleBeep() { for (int i = 0; i < 3; i++) { digitalWrite(BUZZER, HIGH); delay(100); digitalWrite(BUZZER, LOW); delay(100); } }

// === DINO GAME ===
int dinoLevel = 1;
int highScoreDino = 0;
int flyingCactusX = SCREEN_WIDTH + 40;

void runDino() {
  while (true) {
    if (digitalRead(BTN_SELECT) == LOW) {
      currentGame = MENU;
      return;
    }

    display.clearDisplay();

    if (!jumping && digitalRead(BTN_RIGHT) == LOW) {
      jumping = true;
      jumpVel = -8;
    }

    if (jumping) {
      dinoY += jumpVel;
      jumpVel++;
      if (dinoY >= 48) {
        dinoY = 48;
        jumping = false;
      }
    }

    dinoLevel = dinoScore / 5 + 1;
    cactusX -= 4 + dinoLevel;

    if (cactusX < 0) {
      cactusX = SCREEN_WIDTH;
      dinoScore++;
      if (dinoScore > highScoreDino) highScoreDino = dinoScore;
    }

    // Flying cactus from level 3
    if (dinoLevel >= 3) {
      flyingCactusX -= 4 + dinoLevel;
      if (flyingCactusX < 0) flyingCactusX = SCREEN_WIDTH + random(30, 80);

      // Collision with flying cactus
      if (flyingCactusX < 10 && dinoY < 30) {
        tripleBeep();
        gameOverScreen("DINO", dinoScore);
        return;
      }

      display.fillTriangle(flyingCactusX, 30, flyingCactusX + 6, 30, flyingCactusX + 3, 20, SSD1306_WHITE);
    }

    // Collision with ground cactus
    if (cactusX < 10 && dinoY > 40) {
      tripleBeep();
      gameOverScreen("DINO", dinoScore);
      return;
    }

    // Draw Dino and cactus
    display.fillRect(5, dinoY, 8, 12, SSD1306_WHITE);
    display.fillTriangle(cactusX, 60, cactusX + 6, 60, cactusX + 3, 48, SSD1306_WHITE);

    // Display Score & Level
    display.setCursor(0, 0);
    display.print("Score:");
    display.print(dinoScore);
    display.setCursor(80, 0);
    display.print("Lv:");
    display.print(dinoLevel);
    display.setCursor(0, 10);
    display.print("Hi:");
    display.print(highScoreDino);

    display.display();
    delay(50);
  }
}


// === SPACE GAME ===
void runSpace() {
  if (spaceGameOver) { tripleBeep(); currentGame = MENU; return; }
  if (digitalRead(BTN_LEFT) == LOW && playerX > 6) playerX -= 4;
  if (digitalRead(BTN_RIGHT) == LOW && playerX < SCREEN_WIDTH - 6) playerX += 4;
  if (digitalRead(BTN_SELECT) == LOW && !bulletActive) {
    bulletActive = true;
    bulletX = playerX;
    bulletY = playerY;
    beep(1500, 50);
  }

  if (bulletActive) {
    bulletY -= 5;
    if (bulletY < 0) bulletActive = false;
    if (bulletY <= enemyY && bulletX >= enemyX - 5 && bulletX <= enemyX + 5) {
      bulletActive = false;
      enemyY = 0;
      enemyX = random(10, SCREEN_WIDTH - 10);
      spaceScore++;
      beep(1000, 80);
    }
  }

  enemyY += 2;
  if (enemyY >= playerY) spaceGameOver = true;

  display.clearDisplay();
  display.setCursor(0, 0);
  display.print("Score: ");
  display.print(spaceScore);
  display.drawTriangle(playerX, playerY, playerX - 5, playerY + 5, playerX + 5, playerY + 5, SSD1306_WHITE);
  display.drawTriangle(enemyX, enemyY, enemyX - 5, enemyY - 5, enemyX + 5, enemyY - 5, SSD1306_WHITE);
  if (bulletActive) display.drawLine(bulletX, bulletY, bulletX, bulletY - 3, SSD1306_WHITE);
  display.display();
  delay(40);
}

// === FLAPPY BIRD ===
void runFlappy() {
  while (!flappyGameOver) {
    if (digitalRead(BTN_RIGHT) == LOW) birdVel = -5;
    birdVel++;
    birdY += birdVel;

    pipeX -= 3;
    if (pipeX < 0) {
      pipeX = SCREEN_WIDTH;
      gapY = random(20, 40);
      flappyScore++;
    }

    if (birdY > SCREEN_HEIGHT || birdY < 0 || (pipeX < 20 && (birdY < gapY || birdY > gapY + 20))) {
      flappyGameOver = true;
      tripleBeep();
      currentGame = MENU;
      return;
    }

    display.clearDisplay();
    display.setCursor(0, 0);
    display.print("Score: ");
    display.print(flappyScore);
    display.fillCircle(20, birdY, 3, SSD1306_WHITE);
    display.fillRect(pipeX, 0, 10, gapY, SSD1306_WHITE);
    display.fillRect(pipeX, gapY + 20, 10, SCREEN_HEIGHT - gapY - 20, SSD1306_WHITE);
    display.display();
    delay(60);
  }
}

// === PONG ===
void runPong() {
  if (digitalRead(BTN_LEFT) == LOW && paddle1Y > 0) paddle1Y -= 2;
  if (digitalRead(BTN_RIGHT) == LOW && paddle1Y < SCREEN_HEIGHT - 16) paddle1Y += 2;
  paddle2Y = ballY - 8;

  ballX += ballVelX;
  ballY += ballVelY;

  if (ballY <= 0 || ballY >= SCREEN_HEIGHT) ballVelY *= -1;
  if (ballX <= 4 && ballY >= paddle1Y && ballY <= paddle1Y + 16) ballVelX *= -1;
  if (ballX >= SCREEN_WIDTH - 4 && ballY >= paddle2Y && ballY <= paddle2Y + 16) ballVelX *= -1;

  if (ballX <= 0 || ballX >= SCREEN_WIDTH) {
    tripleBeep();
    currentGame = MENU;
    return;
  }

  display.clearDisplay();
  display.fillRect(2, paddle1Y, 2, 16, SSD1306_WHITE);
  display.fillRect(SCREEN_WIDTH - 4, paddle2Y, 2, 16, SSD1306_WHITE);
  display.fillCircle(ballX, ballY, 2, SSD1306_WHITE);
  display.display();
  delay(30);
}

// === MAZE ===
void runMaze() {
  if (digitalRead(BTN_LEFT) == LOW && mazeX > 0) mazeX -= 2;
  if (digitalRead(BTN_RIGHT) == LOW && mazeX < SCREEN_WIDTH - 4) mazeX += 2;
  if (digitalRead(BTN_SELECT) == LOW && mazeY < SCREEN_HEIGHT - 4) mazeY += 2;

  if (mazeX > goalX - 5 && mazeY > goalY - 5) {
    beep(2000, 300);
    currentGame = MENU;
    return;
  }

  display.clearDisplay();
  display.drawRect(goalX, goalY, 8, 8, SSD1306_WHITE);
  display.fillRect(mazeX, mazeY, 4, 4, SSD1306_WHITE);
  display.display();
  delay(60);
}

// === BRICK BREAKER ===
void runBrick() {
  if (digitalRead(BTN_LEFT) == LOW && paddleX > 0) paddleX -= 3;
  if (digitalRead(BTN_RIGHT) == LOW && paddleX < SCREEN_WIDTH - 20) paddleX += 3;

  bX += bVelX;
  bY += bVelY;

  if (bX <= 0 || bX >= SCREEN_WIDTH) bVelX *= -1;
  if (bY <= 0) bVelY *= -1;
  if (bY >= SCREEN_HEIGHT) {
    tripleBeep();
    currentGame = MENU;
    return;
  }

  if (bY >= 58 && bX >= paddleX && bX <= paddleX + 20) bVelY *= -1;

  for (int i = 0; i < 5; i++) {
    for (int j = 0; j < 8; j++) {
      if (bricks[i][j]) {
        int bx = j * 16;
        int by = i * 8;
        if (bX >= bx && bX <= bx + 16 && bY >= by && bY <= by + 8) {
          bricks[i][j] = false;
          bVelY *= -1;
          brickScore++;
          beep(1200, 60);
        }
      }
    }
  }

  display.clearDisplay();
  display.fillRect(paddleX, 60, 20, 4, SSD1306_WHITE);
  display.fillCircle(bX, bY, 2, SSD1306_WHITE);
  for (int i = 0; i < 5; i++) for (int j = 0; j < 8; j++) if (bricks[i][j]) display.fillRect(j * 16, i * 8, 14, 6, SSD1306_WHITE);
  display.display();
  delay(30);
}
