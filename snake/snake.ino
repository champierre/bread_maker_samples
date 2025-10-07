#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

const int LEFT_BUTTON_PIN = 5;
const int RIGHT_BUTTON_PIN = 6;

// ゲーム設定
const int gridSize = 4;
const int gridWidth = SCREEN_WIDTH / gridSize;
const int gridHeight = SCREEN_HEIGHT / gridSize;

// スネーク
const int maxSnakeLength = 100;
int snakeX[maxSnakeLength];
int snakeY[maxSnakeLength];
int snakeLength = 3;

// 方向 (0:右, 1:下, 2:左, 3:上)
int direction = 0;
int nextDirection = 0;

// ゲーム状態
bool gameStarted = false;
bool gameOver = false;
unsigned long lastMoveTime = 0;
int moveDelay = 300;  // 移動速度（ミリ秒）
unsigned long lastButtonTime = 0;
const int buttonDelay = 100;

void setup() {
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
  pinMode(LEFT_BUTTON_PIN, INPUT_PULLUP);
  pinMode(RIGHT_BUTTON_PIN, INPUT_PULLUP);

  randomSeed(analogRead(0));

  displayTitle();
}

void displayTitle() {
  display.clearDisplay();
  display.setTextSize(2);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(25, 15);
  display.print("Snake");
  display.setCursor(25, 35);
  display.print("Game");
  display.setTextSize(1);
  display.setCursor(10, 55);
  display.print("Press L to start");
  display.display();
}

void initGame() {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(25, 28);
  display.print("GAME START!");
  display.display();
  delay(1000);

  // スネークの初期位置（中央、3マス、右向き）
  snakeLength = 3;
  for (int i = 0; i < snakeLength; i++) {
    snakeX[i] = gridWidth / 2 - i;
    snakeY[i] = gridHeight / 2;
  }
  direction = 0;  // 右向き
  nextDirection = 0;
  gameOver = false;
}

void displayGameOver() {
  display.clearDisplay();
  display.setTextSize(2);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(15, 15);
  display.print("GAME");
  display.setCursor(15, 35);
  display.print("OVER");
  display.setTextSize(1);
  display.setCursor(10, 55);
  display.print("Press L to retry");
  display.display();
}

void loop() {
  // ゲーム開始前
  if (!gameStarted) {
    if (digitalRead(LEFT_BUTTON_PIN) == LOW) {
      delay(50);
      while (digitalRead(LEFT_BUTTON_PIN) == LOW) {
        delay(10);
      }
      gameStarted = true;
      initGame();
    }
    return;
  }

  // ゲームオーバー
  if (gameOver) {
    displayGameOver();
    if (digitalRead(LEFT_BUTTON_PIN) == LOW) {
      delay(50);
      while (digitalRead(LEFT_BUTTON_PIN) == LOW) {
        delay(10);
      }
      initGame();
    }
    return;
  }

  // ゲーム中の処理
  unsigned long currentTime = millis();

  // ボタン入力で方向転換（左ボタンで左回転、右ボタンで右回転）
  if (currentTime - lastButtonTime > buttonDelay) {
    if (digitalRead(LEFT_BUTTON_PIN) == LOW) {
      nextDirection = (direction + 3) % 4;  // 左回転
      lastButtonTime = currentTime;
    }
    if (digitalRead(RIGHT_BUTTON_PIN) == LOW) {
      nextDirection = (direction + 1) % 4;  // 右回転
      lastButtonTime = currentTime;
    }
  }

  // スネークの移動
  if (currentTime - lastMoveTime > moveDelay) {
    direction = nextDirection;

    // 新しい頭の位置を計算
    int newHeadX = snakeX[0];
    int newHeadY = snakeY[0];

    switch (direction) {
      case 0: newHeadX++; break;  // 右
      case 1: newHeadY++; break;  // 下
      case 2: newHeadX--; break;  // 左
      case 3: newHeadY--; break;  // 上
    }

    // 壁との衝突判定
    if (newHeadX < 0 || newHeadX >= gridWidth ||
        newHeadY < 0 || newHeadY >= gridHeight) {
      gameOver = true;
      lastMoveTime = currentTime;
      return;
    }

    // 体を移動（後ろから前へ）
    for (int i = snakeLength - 1; i > 0; i--) {
      snakeX[i] = snakeX[i - 1];
      snakeY[i] = snakeY[i - 1];
    }

    // 頭を新しい位置に
    snakeX[0] = newHeadX;
    snakeY[0] = newHeadY;

    lastMoveTime = currentTime;
  }

  // 描画
  display.clearDisplay();

  // スネークを描画
  for (int i = 0; i < snakeLength; i++) {
    if (i == 0) {
      // 頭は少し小さく（中央に描画）
      display.fillRect(snakeX[i] * gridSize + 1, snakeY[i] * gridSize + 1,
                        gridSize - 2, gridSize - 2, SSD1306_WHITE);
    } else {
      // 体
      display.fillRect(snakeX[i] * gridSize, snakeY[i] * gridSize,
                        gridSize - 1, gridSize - 1, SSD1306_WHITE);
    }
  }

  display.display();
  delay(10);
}
