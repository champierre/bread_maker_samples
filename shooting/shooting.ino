#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

const int LEFT_BUTTON_PIN = 5;
const int RIGHT_BUTTON_PIN = 6;

// プレイヤー
int playerX = 54;
const int playerY = 56;
const int playerWidth = 8;
const int playerHeight = 5;

// 弾
struct Bullet {
  float x;
  float y;
  bool active;
};

const int maxBullets = 5;
Bullet bullets[maxBullets];
const int bulletSpeed = 3;
unsigned long lastShot = 0;
const int shootDelay = 300;  // 0.3秒ごとに自動発射

// 敵
const int maxEnemies = 8;
struct Enemy {
  float x;
  float y;
  bool active;
};
Enemy enemies[maxEnemies];
const int enemyWidth = 6;
const int enemyHeight = 5;
unsigned long lastEnemySpawn = 0;
const int enemySpawnDelay = 1500;  // 1.5秒ごとに敵を生成
float enemySpeed = 1.0;

int score = 0;
bool gameOver = false;
bool gameStarted = false;
unsigned long lastButtonPress = 0;

void setup() {
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
  pinMode(LEFT_BUTTON_PIN, INPUT_PULLUP);
  pinMode(RIGHT_BUTTON_PIN, INPUT_PULLUP);

  initGame();
  showTitleScreen();
}

void initGame() {
  // 弾の初期化
  for (int i = 0; i < maxBullets; i++) {
    bullets[i].active = false;
  }

  // 敵の初期化
  for (int i = 0; i < maxEnemies; i++) {
    enemies[i].active = false;
  }

  playerX = 54;
  score = 0;
  gameOver = false;
  gameStarted = false;
  enemySpeed = 1.0;
}

void showTitleScreen() {
  display.clearDisplay();
  display.setTextSize(2);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(20, 10);
  display.print("SHOOTING");

  display.setTextSize(1);
  display.setCursor(10, 35);
  display.print("Press any button");
  display.setCursor(20, 48);
  display.print("to start");

  display.display();
}

void loop() {
  // タイトル画面
  if (!gameStarted) {
    if (digitalRead(LEFT_BUTTON_PIN) == LOW || digitalRead(RIGHT_BUTTON_PIN) == LOW) {
      delay(50);
      while (digitalRead(LEFT_BUTTON_PIN) == LOW || digitalRead(RIGHT_BUTTON_PIN) == LOW) {
        delay(10);
      }
      gameStarted = true;
      lastEnemySpawn = millis();
      lastShot = millis();
    }
    return;
  }

  // ゲームオーバー画面
  if (gameOver) {
    displayGameOver();
    // 左ボタンでリスタート
    if (digitalRead(LEFT_BUTTON_PIN) == LOW) {
      delay(50);
      while (digitalRead(LEFT_BUTTON_PIN) == LOW) {
        delay(10);
      }
      initGame();
      showTitleScreen();
    }
    return;
  }

  unsigned long currentTime = millis();

  // プレイヤー操作
  if (digitalRead(LEFT_BUTTON_PIN) == LOW && playerX > 0) {
    if (currentTime - lastButtonPress > 30) {
      playerX -= 2;
      lastButtonPress = currentTime;
    }
  }
  if (digitalRead(RIGHT_BUTTON_PIN) == LOW) {
    if (currentTime - lastButtonPress > 30) {
      if (playerX < SCREEN_WIDTH - playerWidth) {
        playerX += 2;
      }
      lastButtonPress = currentTime;
    }
  }

  // 自動発射
  if (currentTime - lastShot > shootDelay) {
    // 空いている弾スロットを探す
    for (int i = 0; i < maxBullets; i++) {
      if (!bullets[i].active) {
        bullets[i].x = playerX + playerWidth / 2;
        bullets[i].y = playerY - 2;
        bullets[i].active = true;
        lastShot = currentTime;
        break;
      }
    }
  }

  // 弾の移動
  for (int i = 0; i < maxBullets; i++) {
    if (bullets[i].active) {
      bullets[i].y -= bulletSpeed;
      if (bullets[i].y < 0) {
        bullets[i].active = false;
      }
    }
  }

  // 敵の生成
  if (currentTime - lastEnemySpawn > enemySpawnDelay) {
    for (int i = 0; i < maxEnemies; i++) {
      if (!enemies[i].active) {
        enemies[i].x = random(0, SCREEN_WIDTH - enemyWidth);
        enemies[i].y = 0;
        enemies[i].active = true;
        lastEnemySpawn = currentTime;
        break;
      }
    }
  }

  // 敵の移動
  for (int i = 0; i < maxEnemies; i++) {
    if (enemies[i].active) {
      enemies[i].y += enemySpeed;

      // 画面下に到達したらゲームオーバー
      if (enemies[i].y >= SCREEN_HEIGHT - 10) {
        gameOver = true;
      }

      // 画面外に出たら非アクティブ化
      if (enemies[i].y > SCREEN_HEIGHT) {
        enemies[i].active = false;
      }
    }
  }

  // 弾と敵の衝突判定
  for (int i = 0; i < maxBullets; i++) {
    if (bullets[i].active) {
      for (int j = 0; j < maxEnemies; j++) {
        if (enemies[j].active) {
          if (bullets[i].x >= enemies[j].x &&
              bullets[i].x <= enemies[j].x + enemyWidth &&
              bullets[i].y >= enemies[j].y &&
              bullets[i].y <= enemies[j].y + enemyHeight) {
            enemies[j].active = false;
            bullets[i].active = false;
            score += 10;

            // スコアが上がるごとに敵のスピードを少し上げる
            if (score % 50 == 0) {
              enemySpeed += 0.1;
            }
          }
        }
      }
    }
  }

  // 描画
  display.clearDisplay();

  // 敵の描画
  for (int i = 0; i < maxEnemies; i++) {
    if (enemies[i].active) {
      drawEnemy((int)enemies[i].x, (int)enemies[i].y);
    }
  }

  // プレイヤーの描画
  drawPlayer(playerX, playerY);

  // 弾の描画
  for (int i = 0; i < maxBullets; i++) {
    if (bullets[i].active) {
      display.fillRect((int)bullets[i].x, (int)bullets[i].y, 2, 4, SSD1306_WHITE);
    }
  }

  // スコア表示
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.print("Score:");
  display.print(score);

  display.display();
  delay(20);
}

void drawPlayer(int x, int y) {
  // 三角形の自機
  display.fillTriangle(
    x + playerWidth/2, y,
    x, y + playerHeight,
    x + playerWidth, y + playerHeight,
    SSD1306_WHITE
  );
}

void drawEnemy(int x, int y) {
  // シンプルな敵の形
  display.fillRect(x + 1, y, 4, 2, SSD1306_WHITE);
  display.fillRect(x, y + 2, 6, 2, SSD1306_WHITE);
  display.drawPixel(x, y + 4, SSD1306_WHITE);
  display.drawPixel(x + 5, y + 4, SSD1306_WHITE);
}

void displayGameOver() {
  display.clearDisplay();
  display.setTextSize(2);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(10, 10);
  display.print("GAME OVER");

  display.setTextSize(1);
  display.setCursor(30, 35);
  display.print("Score: ");
  display.print(score);

  display.setCursor(10, 50);
  display.print("LEFT to restart");

  display.display();
}
