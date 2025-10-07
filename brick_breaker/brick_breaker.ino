#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

const int LEFT_BUTTON_PIN = 5;
const int RIGHT_BUTTON_PIN = 6;

// ゲーム変数
int paddleX = 54;          // パドルのX位置
const int paddleY = 58;    // パドルのY位置
const int paddleWidth = 20;
const int paddleHeight = 3;

float ballX = 64;          // ボールのX位置
float ballY = 40;          // ボールのY位置
float ballSpeedX = 1.5;    // ボールのX方向速度
float ballSpeedY = -1.5;   // ボールのY方向速度
const int ballSize = 2;

// ブロック
const int blockRows = 4;
const int blockCols = 8;
const int blockWidth = 14;
const int blockHeight = 4;
bool blocks[blockRows][blockCols];

int score = 0;
bool gameOver = false;

void setup() {
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
  pinMode(LEFT_BUTTON_PIN, INPUT_PULLUP);
  pinMode(RIGHT_BUTTON_PIN, INPUT_PULLUP);

  // ブロックを初期化
  for (int i = 0; i < blockRows; i++) {
    for (int j = 0; j < blockCols; j++) {
      blocks[i][j] = true;
    }
  }
}

void loop() {
  if (gameOver) {
    displayGameOver();
    // REDボタンでリスタート
    if (digitalRead(LEFT_BUTTON_PIN) == LOW) {
      delay(50);  // デバウンス
      while (digitalRead(LEFT_BUTTON_PIN) == LOW) {
        // ボタンが離されるまで待つ
        delay(10);
      }
      resetGame();
    }
    return;
  }

  // ボタン入力
  if (digitalRead(LEFT_BUTTON_PIN) == LOW && paddleX > 0) {
    paddleX -= 3;
  }
  if (digitalRead(RIGHT_BUTTON_PIN) == LOW && paddleX < SCREEN_WIDTH - paddleWidth) {
    paddleX += 3;
  }

  // ボールの移動
  ballX += ballSpeedX;
  ballY += ballSpeedY;

  // 壁との衝突（左右）
  if (ballX <= 0 || ballX >= SCREEN_WIDTH - ballSize) {
    ballSpeedX = -ballSpeedX;
  }

  // 壁との衝突（上）
  if (ballY <= 0) {
    ballSpeedY = -ballSpeedY;
  }

  // パドルとの衝突
  if (ballY >= paddleY - ballSize && ballY <= paddleY + paddleHeight &&
      ballX >= paddleX && ballX <= paddleX + paddleWidth) {
    ballSpeedY = -abs(ballSpeedY);
    // パドルの端で跳ね返った場合は角度を変える
    float hitPos = (ballX - paddleX) / (float)paddleWidth;
    ballSpeedX = (hitPos - 0.5) * 3;
  }

  // ブロックとの衝突
  for (int i = 0; i < blockRows; i++) {
    for (int j = 0; j < blockCols; j++) {
      if (blocks[i][j]) {
        int blockX = j * (blockWidth + 2) + 2;
        int blockY = i * (blockHeight + 2) + 8;

        if (ballX + ballSize >= blockX && ballX <= blockX + blockWidth &&
            ballY + ballSize >= blockY && ballY <= blockY + blockHeight) {
          blocks[i][j] = false;
          ballSpeedY = -ballSpeedY;
          score++;
        }
      }
    }
  }

  // ゲームオーバー判定
  if (ballY >= SCREEN_HEIGHT) {
    gameOver = true;
  }

  // 全ブロック破壊でクリア
  if (score >= blockRows * blockCols) {
    gameOver = true;
  }

  // 描画
  display.clearDisplay();

  // ブロック描画
  for (int i = 0; i < blockRows; i++) {
    for (int j = 0; j < blockCols; j++) {
      if (blocks[i][j]) {
        int blockX = j * (blockWidth + 2) + 2;
        int blockY = i * (blockHeight + 2) + 8;
        display.fillRect(blockX, blockY, blockWidth, blockHeight, SSD1306_WHITE);
      }
    }
  }

  // パドル描画
  display.fillRect(paddleX, paddleY, paddleWidth, paddleHeight, SSD1306_WHITE);

  // ボール描画
  display.fillRect((int)ballX, (int)ballY, ballSize, ballSize, SSD1306_WHITE);

  // スコア表示
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.print("Score:");
  display.print(score);

  display.display();
  delay(20);
}

void displayGameOver() {
  display.clearDisplay();
  display.setTextSize(2);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(10, 20);

  if (score >= blockRows * blockCols) {
    display.print("CLEAR!");
  } else {
    display.print("GAME OVER");
  }

  display.setTextSize(1);
  display.setCursor(20, 45);
  display.print("Score: ");
  display.print(score);

  display.setCursor(10, 55);
  display.print("Press LEFT to start");
  display.display();
}

void resetGame() {
  // ゲーム状態をリセット
  paddleX = 54;
  ballX = 64;
  ballY = 40;
  ballSpeedX = 1.5;
  ballSpeedY = -1.5;
  score = 0;
  gameOver = false;

  // ブロックを再初期化
  for (int i = 0; i < blockRows; i++) {
    for (int j = 0; j < blockCols; j++) {
      blocks[i][j] = true;
    }
  }
}
