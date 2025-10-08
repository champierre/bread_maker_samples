#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET    -1
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// ボタンピン設定
const int LEFT_BUTTON_PIN = 5;
const int RIGHT_BUTTON_PIN = 6;

// ゲーム設定
#define BIRD_X 20
#define BIRD_SIZE 4
#define GRAVITY 0.3
#define JUMP_STRENGTH -3.5
#define PIPE_WIDTH 8
#define PIPE_GAP 20
#define PIPE_SPEED 2
#define MAX_PIPES 3

// ゲーム変数
bool gameStarted = false;
bool gameOver = false;
float birdY = 32;
float birdVelocity = 0;
int score = 0;
int highScore = 0;

// パイプ構造体
struct Pipe {
  int x;
  int gapY;
  bool passed;
};

Pipe pipes[MAX_PIPES];

// 前のボタン状態
bool lastJumpButton = HIGH;
bool lastStartButton = HIGH;

void setup() {
  // ボタン初期化
  pinMode(LEFT_BUTTON_PIN, INPUT_PULLUP);
  pinMode(RIGHT_BUTTON_PIN, INPUT_PULLUP);

  // ディスプレイ初期化
  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    for(;;);
  }

  // ディスプレイ設定
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);

  // タイトル画面表示
  showTitle();
}

void loop() {
  // ゲーム状態の確認
  if (!gameStarted) {
    // タイトル画面処理
    handleTitleScreen();
    return;
  }

  if (gameOver) {
    // ゲームオーバー処理
    handleGameOver();
    return;
  }

  // ゲーム中の処理
  handleGame();

  // フレームレート調整
  delay(30);
}

void showTitle() {
  display.clearDisplay();

  // タイトル
  display.setTextSize(2);
  display.setCursor(20, 10);
  display.print("FLAPPY");
  display.setCursor(30, 28);
  display.print("BIRD");

  // 指示
  display.setTextSize(1);
  display.setCursor(15, 50);
  display.print("Press START!");

  display.display();
}

void handleTitleScreen() {
  bool startButton = digitalRead(RIGHT_BUTTON_PIN);

  // STARTボタンが押された
  if (startButton == LOW && lastStartButton == HIGH) {
    startGame();
  }

  lastStartButton = startButton;
  delay(10);
}

void startGame() {
  gameStarted = true;
  gameOver = false;
  birdY = 32;
  birdVelocity = 0;
  score = 0;

  // パイプ初期化
  initPipes();
}

void initPipes() {
  for (int i = 0; i < MAX_PIPES; i++) {
    pipes[i].x = SCREEN_WIDTH + i * (SCREEN_WIDTH / 2);
    pipes[i].gapY = random(15, SCREEN_HEIGHT - PIPE_GAP - 15);
    pipes[i].passed = false;
  }
}

void handleGame() {
  // 入力処理
  bool jumpButton = digitalRead(LEFT_BUTTON_PIN);
  if (jumpButton == LOW && lastJumpButton == HIGH) {
    birdVelocity = JUMP_STRENGTH;
  }
  lastJumpButton = jumpButton;

  // 物理演算
  birdVelocity += GRAVITY;
  birdY += birdVelocity;

  // 画面境界チェック
  if (birdY < 0 || birdY > SCREEN_HEIGHT - BIRD_SIZE) {
    gameOver = true;
    if (score > highScore) {
      highScore = score;
    }
    return;
  }

  // パイプ更新
  updatePipes();

  // 衝突判定
  if (checkCollision()) {
    gameOver = true;
    if (score > highScore) {
      highScore = score;
    }
    return;
  }

  // 描画
  drawGame();
}

void updatePipes() {
  for (int i = 0; i < MAX_PIPES; i++) {
    // パイプを左に移動
    pipes[i].x -= PIPE_SPEED;

    // スコア加算チェック
    if (!pipes[i].passed && pipes[i].x + PIPE_WIDTH < BIRD_X) {
      pipes[i].passed = true;
      score++;
    }

    // パイプが画面外に出たら右端に再配置
    if (pipes[i].x < -PIPE_WIDTH) {
      pipes[i].x = SCREEN_WIDTH;
      pipes[i].gapY = random(15, SCREEN_HEIGHT - PIPE_GAP - 15);
      pipes[i].passed = false;
    }
  }
}

bool checkCollision() {
  for (int i = 0; i < MAX_PIPES; i++) {
    // 鳥がパイプの横位置にいるか確認
    if (pipes[i].x < BIRD_X + BIRD_SIZE && pipes[i].x + PIPE_WIDTH > BIRD_X) {
      // 鳥がギャップの外にいるか確認
      if (birdY < pipes[i].gapY || birdY + BIRD_SIZE > pipes[i].gapY + PIPE_GAP) {
        return true;
      }
    }
  }
  return false;
}

void drawGame() {
  display.clearDisplay();

  // 鳥を描画
  display.fillRect(BIRD_X, (int)birdY, BIRD_SIZE, BIRD_SIZE, SSD1306_WHITE);

  // パイプを描画
  for (int i = 0; i < MAX_PIPES; i++) {
    // 上のパイプ
    display.fillRect(pipes[i].x, 0, PIPE_WIDTH, pipes[i].gapY, SSD1306_WHITE);
    // 下のパイプ
    display.fillRect(pipes[i].x, pipes[i].gapY + PIPE_GAP, PIPE_WIDTH,
                     SCREEN_HEIGHT - pipes[i].gapY - PIPE_GAP, SSD1306_WHITE);
  }

  // スコア表示
  display.setTextSize(1);
  display.setCursor(5, 5);
  display.print("Score:");
  display.print(score);

  display.display();
}

void handleGameOver() {
  display.clearDisplay();

  // ゲームオーバー表示
  display.setTextSize(2);
  display.setCursor(25, 10);
  display.print("GAME");
  display.setCursor(25, 28);
  display.print("OVER");

  // スコア表示
  display.setTextSize(1);
  display.setCursor(35, 45);
  display.print("Score:");
  display.print(score);

  display.setCursor(30, 55);
  display.print("High:");
  display.print(highScore);

  display.display();

  // 左ボタンでリトライ
  bool retryButton = digitalRead(LEFT_BUTTON_PIN);
  if (retryButton == LOW && lastJumpButton == HIGH) {
    startGame();
  }
  lastJumpButton = retryButton;

  delay(10);
}