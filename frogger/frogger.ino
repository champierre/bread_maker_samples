#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

const int LEFT_BUTTON_PIN = 5;   // 後退
const int RIGHT_BUTTON_PIN = 6;  // 前進

// プレイヤー（カエル）
int playerX = 60;
int playerY = 56;
const int playerSize = 6;
const int moveStep = 8;  // 1回の移動量

// 車のレーン
const int numLanes = 6;
const int laneHeight = 8;
const int laneY[] = {8, 16, 24, 32, 40, 48};  // 各レーンのY座標

// 車の構造体
struct Car {
  float x;
  int laneIndex;
  int width;
  bool active;
};

const int maxCars = 10;
Car cars[maxCars];
float carSpeeds[] = {1.0, -1.5, 1.2, -0.8, 1.8, -1.0};  // 各レーンの速度（負は左向き）

// ゲーム状態
bool gameStarted = false;
bool gameOver = false;
unsigned long lastButtonPress = 0;
const int buttonDelay = 200;
int score = 0;
int level = 1;
float speedMultiplier = 1.0;

void setup() {
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
  pinMode(LEFT_BUTTON_PIN, INPUT_PULLUP);
  pinMode(RIGHT_BUTTON_PIN, INPUT_PULLUP);

  initGame();
  displayTitleScreen();
}

void initGame() {
  playerX = 60;
  playerY = 56;
  gameOver = false;
  score = 0;
  level = 1;
  speedMultiplier = 1.0;

  // 車の初期化
  for (int i = 0; i < maxCars; i++) {
    cars[i].laneIndex = i % numLanes;
    cars[i].width = 12 + random(8);  // 車の幅をランダムに

    // 速度に応じて初期位置を設定
    if (carSpeeds[cars[i].laneIndex] > 0) {
      cars[i].x = -20 - (i * 30);  // 左から来る車
    } else {
      cars[i].x = SCREEN_WIDTH + 20 + (i * 30);  // 右から来る車
    }
    cars[i].active = true;
  }
}

void resetPlayerPosition() {
  playerX = 60;
  playerY = 56;
}

void displayTitleScreen() {
  display.clearDisplay();
  display.setTextSize(2);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(20, 10);
  display.print("FROGGER");

  display.setTextSize(1);
  display.setCursor(10, 35);
  display.print("RIGHT: Forward");
  display.setCursor(10, 45);
  display.print("LEFT: Backward");
  display.setCursor(10, 55);
  display.print("RIGHT to start");

  display.display();
}

void loop() {
  unsigned long currentTime = millis();

  // タイトル画面
  if (!gameStarted) {
    if (digitalRead(RIGHT_BUTTON_PIN) == LOW) {
      delay(50);
      while (digitalRead(RIGHT_BUTTON_PIN) == LOW) {
        delay(10);
      }
      gameStarted = true;
      initGame();
    }
    return;
  }

  // ゲームオーバー画面
  if (gameOver) {
    displayEndScreen();
    if (digitalRead(LEFT_BUTTON_PIN) == LOW) {
      delay(50);
      while (digitalRead(LEFT_BUTTON_PIN) == LOW) {
        delay(10);
      }
      gameStarted = false;
      displayTitleScreen();
    }
    return;
  }

  // プレイヤー操作
  if (currentTime - lastButtonPress > buttonDelay) {
    // 右ボタンで前進（上に移動）
    if (digitalRead(RIGHT_BUTTON_PIN) == LOW) {
      if (playerY > 0) {
        playerY -= moveStep;
        if (playerY < 0) playerY = 0;  // 上限を超えないように
        score += 10;
        lastButtonPress = currentTime;

        // ゴール到達チェック
        if (playerY <= 6) {
          // ボーナス獲得
          score += 100;
          level++;
          speedMultiplier += 0.2;  // スピードを20%増加

          // 短いボーナス表示
          display.clearDisplay();
          display.setTextSize(2);
          display.setTextColor(SSD1306_WHITE);
          display.setCursor(30, 20);
          display.print("GOAL!");
          display.setTextSize(1);
          display.setCursor(20, 40);
          display.print("Bonus +100");
          display.display();
          delay(1000);

          // プレイヤーをスタート地点に戻す
          resetPlayerPosition();
        }
      }
      delay(50);
      while (digitalRead(RIGHT_BUTTON_PIN) == LOW) {
        delay(10);
      }
    }

    // 左ボタンで後退（下に移動）
    if (digitalRead(LEFT_BUTTON_PIN) == LOW) {
      // スタート地点（56）より上にいる場合のみ後退可能
      if (playerY < 56 && playerY + moveStep <= 56 - moveStep) {
        playerY += moveStep;
        score = max(0, score - 5);  // 後退するとスコアが減る
        lastButtonPress = currentTime;
      }
      delay(50);
      while (digitalRead(LEFT_BUTTON_PIN) == LOW) {
        delay(10);
      }
    }
  }

  // 車の移動
  for (int i = 0; i < maxCars; i++) {
    if (cars[i].active) {
      int lane = cars[i].laneIndex;
      cars[i].x += carSpeeds[lane] * speedMultiplier;  // スピード倍率を適用

      // 画面外に出たら反対側から再登場
      if (carSpeeds[lane] > 0) {
        if (cars[i].x > SCREEN_WIDTH + 20) {
          cars[i].x = -cars[i].width - 20;
          cars[i].width = 12 + random(8);
        }
      } else {
        if (cars[i].x < -cars[i].width - 20) {
          cars[i].x = SCREEN_WIDTH + 20;
          cars[i].width = 12 + random(8);
        }
      }
    }
  }

  // 衝突判定
  checkCollision();

  // 描画
  display.clearDisplay();

  // ゴールエリア
  display.drawRect(0, 0, SCREEN_WIDTH, 6, SSD1306_WHITE);
  display.setTextSize(1);
  display.setCursor(45, 0);
  display.print("GOAL");

  // レーンの線
  for (int i = 0; i < numLanes; i++) {
    display.drawLine(0, laneY[i] - 1, SCREEN_WIDTH, laneY[i] - 1, SSD1306_WHITE);
  }

  // 車の描画
  for (int i = 0; i < maxCars; i++) {
    if (cars[i].active) {
      drawCar((int)cars[i].x, laneY[cars[i].laneIndex], cars[i].width);
    }
  }

  // プレイヤー（カエル）の描画
  drawFrog(playerX, playerY);

  // スコアとレベル表示
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, SCREEN_HEIGHT - 8);
  display.print("S:");
  display.print(score);
  display.setCursor(80, SCREEN_HEIGHT - 8);
  display.print("Lv:");
  display.print(level);

  display.display();
  delay(20);
}

void drawFrog(int x, int y) {
  // カエルの体
  display.fillRect(x + 1, y + 1, 4, 4, SSD1306_WHITE);
  // 目
  display.drawPixel(x + 1, y, SSD1306_WHITE);
  display.drawPixel(x + 4, y, SSD1306_WHITE);
  // 足
  display.drawPixel(x, y + 2, SSD1306_WHITE);
  display.drawPixel(x + 5, y + 2, SSD1306_WHITE);
}

void drawCar(int x, int y, int width) {
  // 車本体
  display.fillRect(x, y, width, 6, SSD1306_WHITE);
  // 窓
  display.fillRect(x + 2, y + 1, width - 4, 2, SSD1306_BLACK);
}

void checkCollision() {
  for (int i = 0; i < maxCars; i++) {
    if (cars[i].active) {
      int carX = (int)cars[i].x;
      int carY = laneY[cars[i].laneIndex];
      int carWidth = cars[i].width;

      // 衝突判定（矩形同士の重なりチェック）
      if (playerX + playerSize > carX && playerX < carX + carWidth &&
          playerY + playerSize > carY && playerY < carY + 6) {
        gameOver = true;
      }
    }
  }
}

void displayEndScreen() {
  display.clearDisplay();
  display.setTextSize(2);
  display.setTextColor(SSD1306_WHITE);

  display.setCursor(10, 10);
  display.print("GAME OVER");

  display.setTextSize(1);
  display.setCursor(20, 30);
  display.print("Score: ");
  display.print(score);

  display.setCursor(20, 40);
  display.print("Level: ");
  display.print(level);

  display.setCursor(10, 52);
  display.print("LEFT to restart");

  display.display();
}
