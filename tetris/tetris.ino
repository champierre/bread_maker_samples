#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

const int LEFT_BUTTON_PIN = 5;
const int RIGHT_BUTTON_PIN = 6;
const int BUZZER_PIN = 3;

// ゲームフィールド設定
const int FIELD_WIDTH = 10;
const int FIELD_HEIGHT = 16;
const int BLOCK_SIZE = 3;
const int FIELD_OFFSET_X = 2;
const int FIELD_OFFSET_Y = 8;

// フィールド (0=空, 1=ブロック)
byte field[FIELD_HEIGHT][FIELD_WIDTH];

// テトロミノの形状 (4x4の配列)
const byte TETROMINOS[7][4][4] = {
  // I
  {
    {0,0,0,0},
    {1,1,1,1},
    {0,0,0,0},
    {0,0,0,0}
  },
  // O
  {
    {0,0,0,0},
    {0,1,1,0},
    {0,1,1,0},
    {0,0,0,0}
  },
  // T
  {
    {0,0,0,0},
    {0,1,0,0},
    {1,1,1,0},
    {0,0,0,0}
  },
  // S
  {
    {0,0,0,0},
    {0,1,1,0},
    {1,1,0,0},
    {0,0,0,0}
  },
  // Z
  {
    {0,0,0,0},
    {1,1,0,0},
    {0,1,1,0},
    {0,0,0,0}
  },
  // J
  {
    {0,0,0,0},
    {1,0,0,0},
    {1,1,1,0},
    {0,0,0,0}
  },
  // L
  {
    {0,0,0,0},
    {0,0,1,0},
    {1,1,1,0},
    {0,0,0,0}
  }
};

// 現在のテトロミノ
int currentType = 0;
int currentX = 3;
int currentY = 0;
int currentRotation = 0;
byte currentShape[4][4];

// ゲーム状態
bool gameStarted = false;
bool gameOver = false;
bool soundPlayed = false;
int score = 0;
unsigned long lastFallTime = 0;
int fallDelay = 500;
unsigned long lastButtonTime = 0;
const int buttonDelay = 150;

void setup() {
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
  pinMode(LEFT_BUTTON_PIN, INPUT_PULLUP);
  pinMode(RIGHT_BUTTON_PIN, INPUT_PULLUP);
  pinMode(BUZZER_PIN, OUTPUT);

  randomSeed(analogRead(0));

  displayTitle();
}

void displayTitle() {
  display.clearDisplay();
  display.setTextSize(2);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(20, 20);
  display.print("Tetris");
  display.setTextSize(1);
  display.setCursor(10, 50);
  display.print("Press L to start");
  display.display();
}

void initGame() {
  // フィールドをクリア
  for (int y = 0; y < FIELD_HEIGHT; y++) {
    for (int x = 0; x < FIELD_WIDTH; x++) {
      field[y][x] = 0;
    }
  }

  score = 0;
  gameOver = false;
  soundPlayed = false;
  spawnNewTetromino();
}

void spawnNewTetromino() {
  currentType = random(7);
  currentX = 3;
  currentY = 0;
  currentRotation = 0;

  // 形状をコピー
  for (int y = 0; y < 4; y++) {
    for (int x = 0; x < 4; x++) {
      currentShape[y][x] = TETROMINOS[currentType][y][x];
    }
  }

  // スポーン位置に既にブロックがある場合はゲームオーバー
  if (checkCollision(currentX, currentY, currentShape)) {
    gameOver = true;
  }
}

void rotateTetromino() {
  byte newShape[4][4];

  // 90度回転
  for (int y = 0; y < 4; y++) {
    for (int x = 0; x < 4; x++) {
      newShape[x][3-y] = currentShape[y][x];
    }
  }

  // 回転後の衝突チェック
  if (!checkCollision(currentX, currentY, newShape)) {
    for (int y = 0; y < 4; y++) {
      for (int x = 0; x < 4; x++) {
        currentShape[y][x] = newShape[y][x];
      }
    }
  }
}

bool checkCollision(int x, int y, byte shape[4][4]) {
  for (int py = 0; py < 4; py++) {
    for (int px = 0; px < 4; px++) {
      if (shape[py][px]) {
        int fieldX = x + px;
        int fieldY = y + py;

        // 壁のチェック
        if (fieldX < 0 || fieldX >= FIELD_WIDTH || fieldY >= FIELD_HEIGHT) {
          return true;
        }

        // 既存のブロックとの衝突チェック
        if (fieldY >= 0 && field[fieldY][fieldX]) {
          return true;
        }
      }
    }
  }
  return false;
}

void lockTetromino() {
  // テトロミノをフィールドに固定
  for (int py = 0; py < 4; py++) {
    for (int px = 0; px < 4; px++) {
      if (currentShape[py][px]) {
        int fieldY = currentY + py;
        int fieldX = currentX + px;
        if (fieldY >= 0 && fieldY < FIELD_HEIGHT && fieldX >= 0 && fieldX < FIELD_WIDTH) {
          field[fieldY][fieldX] = 1;
        }
      }
    }
  }

  // ライン消去チェック
  clearLines();

  // 新しいテトロミノを生成
  spawnNewTetromino();
}

void clearLines() {
  int linesCleared = 0;

  for (int y = FIELD_HEIGHT - 1; y >= 0; y--) {
    bool lineFull = true;
    for (int x = 0; x < FIELD_WIDTH; x++) {
      if (field[y][x] == 0) {
        lineFull = false;
        break;
      }
    }

    if (lineFull) {
      linesCleared++;

      // ラインを消して上のブロックを落とす
      for (int yy = y; yy > 0; yy--) {
        for (int x = 0; x < FIELD_WIDTH; x++) {
          field[yy][x] = field[yy - 1][x];
        }
      }

      // 一番上の行をクリア
      for (int x = 0; x < FIELD_WIDTH; x++) {
        field[0][x] = 0;
      }

      y++; // 同じ行を再チェック
    }
  }

  if (linesCleared > 0) {
    score += linesCleared * 10;
  }
}

void drawField() {
  // 枠線を描画
  display.drawRect(FIELD_OFFSET_X - 1, FIELD_OFFSET_Y - 1,
                   FIELD_WIDTH * BLOCK_SIZE + 2, FIELD_HEIGHT * BLOCK_SIZE + 2,
                   SSD1306_WHITE);

  // フィールドのブロックを描画
  for (int y = 0; y < FIELD_HEIGHT; y++) {
    for (int x = 0; x < FIELD_WIDTH; x++) {
      if (field[y][x]) {
        display.fillRect(FIELD_OFFSET_X + x * BLOCK_SIZE,
                        FIELD_OFFSET_Y + y * BLOCK_SIZE,
                        BLOCK_SIZE - 1, BLOCK_SIZE - 1, SSD1306_WHITE);
      }
    }
  }

  // 現在のテトロミノを描画
  for (int py = 0; py < 4; py++) {
    for (int px = 0; px < 4; px++) {
      if (currentShape[py][px]) {
        int drawY = currentY + py;
        if (drawY >= 0) {
          display.fillRect(FIELD_OFFSET_X + (currentX + px) * BLOCK_SIZE,
                          FIELD_OFFSET_Y + drawY * BLOCK_SIZE,
                          BLOCK_SIZE - 1, BLOCK_SIZE - 1, SSD1306_WHITE);
        }
      }
    }
  }
}

void playGameOverSound() {
  // ゲームオーバー音を鳴らす（1回だけ）
  tone(BUZZER_PIN, 200, 200);  // 低い音
  delay(250);
  tone(BUZZER_PIN, 150, 200);  // さらに低い音
  delay(250);
  tone(BUZZER_PIN, 100, 400);  // とても低い音
  delay(450);
  noTone(BUZZER_PIN);
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
  display.setCursor(30, 55);
  display.print("Score:");
  display.print(score);
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
    // 音を1回だけ鳴らす
    if (!soundPlayed) {
      playGameOverSound();
      soundPlayed = true;
      delay(1000);  // スコア表示時間
    }

    // リトライ待ち画面を表示
    display.clearDisplay();
    display.setTextSize(1);
    display.setCursor(10, 20);
    display.print("Score:");
    display.print(score);
    display.setCursor(10, 35);
    display.print("Press L to retry");
    display.display();

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

  // ボタン入力
  if (currentTime - lastButtonTime > buttonDelay) {
    bool leftPressed = digitalRead(LEFT_BUTTON_PIN) == LOW;
    bool rightPressed = digitalRead(RIGHT_BUTTON_PIN) == LOW;

    if (leftPressed && rightPressed) {
      // 両方押されたら回転
      rotateTetromino();
      lastButtonTime = currentTime;
    } else if (leftPressed) {
      // 左移動
      if (!checkCollision(currentX - 1, currentY, currentShape)) {
        currentX--;
      }
      lastButtonTime = currentTime;
    } else if (rightPressed) {
      // 右移動
      if (!checkCollision(currentX + 1, currentY, currentShape)) {
        currentX++;
      }
      lastButtonTime = currentTime;
    }
  }

  // 落下
  if (currentTime - lastFallTime > fallDelay) {
    if (!checkCollision(currentX, currentY + 1, currentShape)) {
      currentY++;
    } else {
      lockTetromino();
    }
    lastFallTime = currentTime;
  }

  // 描画
  display.clearDisplay();

  // スコア表示
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.print("S:");
  display.print(score);

  // フィールドとテトロミノを描画
  drawField();

  display.display();
  delay(10);
}
