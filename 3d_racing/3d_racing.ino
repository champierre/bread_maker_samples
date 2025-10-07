#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

const int LEFT_BUTTON_PIN = 5;
const int RIGHT_BUTTON_PIN = 6;

// ゲーム設定
const int ROAD_SEGMENTS = 8;  // 道路の分割数
const int MAX_OBSTACLES = 3;   // 最大障害物数

// プレイヤー
int playerX = 64;  // 画面中央
const int playerY = 52;
const int playerWidth = 6;
const int playerHeight = 8;

// 道路
float roadOffset = 0;
float roadSpeed = 2.0;

// 障害物
struct Obstacle {
  float z;        // 奥行き位置 (0=遠い, 100=手前)
  int laneX;      // レーン位置 (0=左, 1=中央, 2=右)
  bool active;
};
Obstacle obstacles[MAX_OBSTACLES];

// ゲーム状態
bool gameStarted = false;
bool gameOver = false;
int score = 0;
unsigned long lastUpdateTime = 0;
unsigned long lastObstacleTime = 0;
const int obstacleSpawnDelay = 2000;
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
  display.setCursor(35, 15);
  display.print("3D");
  display.setCursor(10, 35);
  display.print("Racing");
  display.setTextSize(1);
  display.setCursor(10, 55);
  display.print("Press L to start");
  display.display();
}

void initGame() {
  playerX = 64;
  roadOffset = 0;
  roadSpeed = 2.0;
  score = 0;
  gameOver = false;

  // 障害物をクリア
  for (int i = 0; i < MAX_OBSTACLES; i++) {
    obstacles[i].active = false;
  }

  lastObstacleTime = millis();
}

void spawnObstacle() {
  // 空いているスロットを探す
  for (int i = 0; i < MAX_OBSTACLES; i++) {
    if (!obstacles[i].active) {
      obstacles[i].z = 0;  // 遠くから開始
      obstacles[i].laneX = random(3);  // 0, 1, 2のいずれか
      obstacles[i].active = true;
      break;
    }
  }
}

void drawRoad() {
  // 道路セグメントを描画（奥から手前へ）
  for (int i = 0; i < ROAD_SEGMENTS; i++) {
    float depth = (float)i / ROAD_SEGMENTS;  // 0(遠い) ～ 1(近い)

    // オフセットを適用
    float segmentOffset = fmod(roadOffset + depth * 100, 20);

    // 遠近法による幅の計算
    int y = 10 + (int)(depth * 50);
    int roadWidth = 20 + (int)(depth * 80);
    int leftX = (SCREEN_WIDTH - roadWidth) / 2;
    int rightX = leftX + roadWidth;

    // 道路の境界線
    if (i % 2 == 0) {
      display.drawFastVLine(leftX, y, 8, SSD1306_WHITE);
      display.drawFastVLine(rightX, y, 8, SSD1306_WHITE);
    }

    // 中央線（点線）
    if (i % 2 == (int)(roadOffset / 10) % 2) {
      int centerX = SCREEN_WIDTH / 2;
      display.drawFastVLine(centerX, y, 4, SSD1306_WHITE);
    }
  }
}

void drawPlayer() {
  // シンプルな車の形
  // 車体
  display.fillRect(playerX - playerWidth/2, playerY, playerWidth, playerHeight, SSD1306_WHITE);
  // フロント部分
  display.fillTriangle(
    playerX - playerWidth/2, playerY,
    playerX + playerWidth/2, playerY,
    playerX, playerY - 3,
    SSD1306_WHITE
  );
}

void drawObstacles() {
  for (int i = 0; i < MAX_OBSTACLES; i++) {
    if (obstacles[i].active) {
      // 奥行きから画面上の位置とサイズを計算
      float depth = obstacles[i].z / 100.0;  // 0(遠い) ～ 1(近い)

      if (depth >= 0 && depth <= 1) {
        int y = 10 + (int)(depth * 50);
        int size = 3 + (int)(depth * 5);

        // レーン位置の計算
        int roadWidth = 20 + (int)(depth * 80);
        int leftX = (SCREEN_WIDTH - roadWidth) / 2;
        int laneWidth = roadWidth / 3;
        int obstacleX = leftX + obstacles[i].laneX * laneWidth + laneWidth / 2;

        // 障害物を描画（四角）
        display.fillRect(obstacleX - size/2, y, size, size * 2, SSD1306_WHITE);
      }
    }
  }
}

bool checkCollision() {
  for (int i = 0; i < MAX_OBSTACLES; i++) {
    if (obstacles[i].active) {
      float depth = obstacles[i].z / 100.0;

      // プレイヤーの近くにある障害物のみチェック
      if (depth > 0.8 && depth <= 1.0) {
        // プレイヤーのレーン位置を計算
        int roadWidth = 20 + (int)(depth * 80);
        int leftX = (SCREEN_WIDTH - roadWidth) / 2;
        int laneWidth = roadWidth / 3;

        // 各レーンの範囲
        for (int lane = 0; lane < 3; lane++) {
          int laneLeft = leftX + lane * laneWidth;
          int laneRight = laneLeft + laneWidth;

          // プレイヤーがこのレーンにいるか
          if (playerX >= laneLeft && playerX <= laneRight) {
            // 障害物も同じレーンにあるか
            if (obstacles[i].laneX == lane) {
              return true;  // 衝突！
            }
          }
        }
      }
    }
  }
  return false;
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
  display.setCursor(25, 55);
  display.print("Score:");
  display.print(score);
  display.display();
  delay(2000);

  display.clearDisplay();
  display.setTextSize(1);
  display.setCursor(10, 28);
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

  unsigned long currentTime = millis();

  // プレイヤー操作
  if (currentTime - lastButtonTime > buttonDelay) {
    if (digitalRead(LEFT_BUTTON_PIN) == LOW) {
      playerX -= 3;
      if (playerX < 20) playerX = 20;
      lastButtonTime = currentTime;
    }
    if (digitalRead(RIGHT_BUTTON_PIN) == LOW) {
      playerX += 3;
      if (playerX > 108) playerX = 108;
      lastButtonTime = currentTime;
    }
  }

  // 道路のスクロール
  roadOffset += roadSpeed;
  if (roadOffset > 100) roadOffset = 0;

  // 障害物の更新
  for (int i = 0; i < MAX_OBSTACLES; i++) {
    if (obstacles[i].active) {
      obstacles[i].z += roadSpeed * 2;

      // 画面外に出たら非アクティブに
      if (obstacles[i].z > 100) {
        obstacles[i].active = false;
        score += 10;  // 避けたらスコア加算
      }
    }
  }

  // 新しい障害物を生成
  if (currentTime - lastObstacleTime > obstacleSpawnDelay) {
    spawnObstacle();
    lastObstacleTime = currentTime;
  }

  // 衝突判定
  if (checkCollision()) {
    gameOver = true;
    return;
  }

  // スコアに応じて速度アップ
  if (score > 0 && score % 50 == 0) {
    roadSpeed = 2.0 + (score / 100.0);
  }

  // 描画
  display.clearDisplay();

  // スコア表示
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.print("S:");
  display.print(score);

  // 速度表示
  display.setCursor(80, 0);
  display.print("SPD:");
  display.print((int)roadSpeed);

  // 道路を描画
  drawRoad();

  // 障害物を描画
  drawObstacles();

  // プレイヤーを描画
  drawPlayer();

  display.display();
  delay(30);
}
