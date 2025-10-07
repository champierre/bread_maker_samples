#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

const int LEFT_BUTTON_PIN = 5;
const int RIGHT_BUTTON_PIN = 6;

// パックマン
int pacmanX = 10;
int pacmanY = 30;
const int pacmanSize = 6;
int direction = 1; // 1=右, -1=左, 2=上, -2=下
unsigned long lastMoveTime = 0;
const int moveDelay = 150;

// ドット
const int maxDots = 20;
struct Dot {
  int x;
  int y;
  bool active;
};
Dot dots[maxDots];

// ゲーム状態
int score = 0;
bool gameStarted = false;
bool gameOver = false;
unsigned long lastButtonPress = 0;
const int buttonDelay = 150;

void setup() {
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
  pinMode(LEFT_BUTTON_PIN, INPUT_PULLUP);
  pinMode(RIGHT_BUTTON_PIN, INPUT_PULLUP);

  initGame();
}

void initGame() {
  pacmanX = 10;
  pacmanY = 30;
  direction = 1;
  score = 0;
  gameStarted = false;
  gameOver = false;

  // ドットの初期化
  for (int i = 0; i < maxDots; i++) {
    dots[i].x = (i % 10) * 12 + 15;
    dots[i].y = (i / 10) * 15 + 20;
    dots[i].active = true;
  }
}

void loop() {
  if (!gameStarted) {
    displayTitleScreen();
    // 左ボタンでゲーム開始
    if (digitalRead(LEFT_BUTTON_PIN) == LOW) {
      delay(50);
      while (digitalRead(LEFT_BUTTON_PIN) == LOW) {
        delay(10);
      }
      gameStarted = true;
    }
    return;
  }

  if (gameOver) {
    displayGameOverScreen();
    // 左ボタンでリスタート
    if (digitalRead(LEFT_BUTTON_PIN) == LOW) {
      delay(50);
      while (digitalRead(LEFT_BUTTON_PIN) == LOW) {
        delay(10);
      }
      initGame();
    }
    return;
  }

  unsigned long currentTime = millis();

  // ボタン入力で方向変更
  if (digitalRead(LEFT_BUTTON_PIN) == LOW) {
    if (currentTime - lastButtonPress > buttonDelay) {
      direction = -1; // 左
      lastButtonPress = currentTime;
    }
  }
  if (digitalRead(RIGHT_BUTTON_PIN) == LOW) {
    if (currentTime - lastButtonPress > buttonDelay) {
      direction = 1; // 右
      lastButtonPress = currentTime;
    }
  }

  // 自動移動
  if (currentTime - lastMoveTime > moveDelay) {
    if (direction == 1) { // 右
      pacmanX += 3;
      if (pacmanX > SCREEN_WIDTH - pacmanSize) {
        pacmanX = 0; // 画面右端から左端へ
      }
    } else if (direction == -1) { // 左
      pacmanX -= 3;
      if (pacmanX < 0) {
        pacmanX = SCREEN_WIDTH - pacmanSize; // 画面左端から右端へ
      }
    }

    lastMoveTime = currentTime;
  }

  // ドットとの衝突判定
  for (int i = 0; i < maxDots; i++) {
    if (dots[i].active) {
      if (abs(pacmanX - dots[i].x) < pacmanSize &&
          abs(pacmanY - dots[i].y) < pacmanSize) {
        dots[i].active = false;
        score += 10;
      }
    }
  }

  // 全ドット取得チェック
  bool anyDotActive = false;
  for (int i = 0; i < maxDots; i++) {
    if (dots[i].active) {
      anyDotActive = true;
      break;
    }
  }
  if (!anyDotActive) {
    gameOver = true;
  }

  // 描画
  display.clearDisplay();

  // ドットの描画
  for (int i = 0; i < maxDots; i++) {
    if (dots[i].active) {
      display.fillCircle(dots[i].x, dots[i].y, 2, SSD1306_WHITE);
    }
  }

  // パックマンの描画
  drawPacman(pacmanX, pacmanY, direction);

  // スコア表示
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.print("Score: ");
  display.print(score);

  display.display();
  delay(20);
}

void drawPacman(int x, int y, int dir) {
  // パックマンの口の向き
  int startAngle = 0;
  int endAngle = 360;

  if (dir == 1) { // 右向き
    display.fillCircle(x + pacmanSize/2, y + pacmanSize/2, pacmanSize/2, SSD1306_WHITE);
    // 口の部分を黒で描画（三角形で口を表現）
    display.fillTriangle(
      x + pacmanSize/2, y + pacmanSize/2,
      x + pacmanSize, y + 1,
      x + pacmanSize, y + pacmanSize - 1,
      SSD1306_BLACK
    );
  } else { // 左向き
    display.fillCircle(x + pacmanSize/2, y + pacmanSize/2, pacmanSize/2, SSD1306_WHITE);
    // 口の部分を黒で描画
    display.fillTriangle(
      x + pacmanSize/2, y + pacmanSize/2,
      x, y + 1,
      x, y + pacmanSize - 1,
      SSD1306_BLACK
    );
  }
}

void displayTitleScreen() {
  display.clearDisplay();
  display.setTextSize(2);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(15, 15);
  display.print("PAC-MAN");

  display.setTextSize(1);
  display.setCursor(10, 40);
  display.print("LEFT button to start");

  display.setCursor(10, 52);
  display.print("L/R buttons to turn");

  display.display();
}

void displayGameOverScreen() {
  display.clearDisplay();
  display.setTextSize(2);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(20, 15);
  display.print("CLEAR!");

  display.setTextSize(1);
  display.setCursor(30, 35);
  display.print("Score: ");
  display.print(score);

  display.setCursor(10, 50);
  display.print("LEFT to restart");

  display.display();
}