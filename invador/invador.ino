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

Bullet playerBullet = {0, 0, false};
const int bulletSpeed = 3;
unsigned long lastPlayerShot = 0;
const int playerShootDelay = 500;  // 0.5秒ごとに自動発射

// 敵
const int enemyRows = 3;
const int enemyCols = 8;
const int enemyWidth = 6;
const int enemyHeight = 5;
const int enemySpacing = 4;
struct Enemy {
  float x;
  float y;
  bool alive;
};
Enemy enemies[enemyRows][enemyCols];
float enemyDirection = 1;
float enemySpeed = 0.5;
unsigned long lastEnemyMove = 0;
const int enemyMoveDelay = 200;

// 敵の弾
const int maxEnemyBullets = 3;
Bullet enemyBullets[maxEnemyBullets];
unsigned long lastEnemyShot = 0;
const int enemyShootDelay = 1500;

int score = 0;
bool gameOver = false;
bool gameWin = false;
unsigned long lastButtonPress = 0;
const int buttonDelay = 150;

void setup() {
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
  pinMode(LEFT_BUTTON_PIN, INPUT_PULLUP);
  pinMode(RIGHT_BUTTON_PIN, INPUT_PULLUP);

  initGame();
}

void initGame() {
  // 敵の初期化
  for (int i = 0; i < enemyRows; i++) {
    for (int j = 0; j < enemyCols; j++) {
      enemies[i][j].x = j * (enemyWidth + enemySpacing) + 8;
      enemies[i][j].y = i * (enemyHeight + enemySpacing) + 5;
      enemies[i][j].alive = true;
    }
  }

  // 敵の弾の初期化
  for (int i = 0; i < maxEnemyBullets; i++) {
    enemyBullets[i].active = false;
  }

  playerX = 54;
  playerBullet.active = false;
  score = 0;
  gameOver = false;
  gameWin = false;
  enemyDirection = 1;
  enemySpeed = 0.5;
}

void loop() {
  if (gameOver || gameWin) {
    displayEndScreen();
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
  if (!playerBullet.active && currentTime - lastPlayerShot > playerShootDelay) {
    playerBullet.x = playerX + playerWidth / 2;
    playerBullet.y = playerY - 2;
    playerBullet.active = true;
    lastPlayerShot = currentTime;
  }

  // プレイヤーの弾の移動
  if (playerBullet.active) {
    playerBullet.y -= bulletSpeed;
    if (playerBullet.y < 0) {
      playerBullet.active = false;
    }
  }

  // 敵の移動
  if (currentTime - lastEnemyMove > enemyMoveDelay) {
    bool needMoveDown = false;

    // 端に到達したかチェック
    for (int i = 0; i < enemyRows; i++) {
      for (int j = 0; j < enemyCols; j++) {
        if (enemies[i][j].alive) {
          if ((enemies[i][j].x <= 0 && enemyDirection < 0) ||
              (enemies[i][j].x >= SCREEN_WIDTH - enemyWidth && enemyDirection > 0)) {
            needMoveDown = true;
            enemyDirection = -enemyDirection;
            break;
          }
        }
      }
      if (needMoveDown) break;
    }

    // 敵を移動
    for (int i = 0; i < enemyRows; i++) {
      for (int j = 0; j < enemyCols; j++) {
        if (enemies[i][j].alive) {
          enemies[i][j].x += enemyDirection * enemySpeed;
          if (needMoveDown) {
            enemies[i][j].y += 3;
            // 敵がプレイヤーまで到達したらゲームオーバー
            if (enemies[i][j].y >= playerY - enemyHeight) {
              gameOver = true;
            }
          }
        }
      }
    }

    lastEnemyMove = currentTime;
  }

  // 敵の発射
  if (currentTime - lastEnemyShot > enemyShootDelay) {
    // ランダムな生きている敵から発射
    int attempts = 0;
    while (attempts < 20) {
      int i = random(enemyRows);
      int j = random(enemyCols);
      if (enemies[i][j].alive) {
        // 空いている弾スロットを探す
        for (int b = 0; b < maxEnemyBullets; b++) {
          if (!enemyBullets[b].active) {
            enemyBullets[b].x = enemies[i][j].x + enemyWidth / 2;
            enemyBullets[b].y = enemies[i][j].y + enemyHeight;
            enemyBullets[b].active = true;
            break;
          }
        }
        break;
      }
      attempts++;
    }
    lastEnemyShot = currentTime;
  }

  // 敵の弾の移動
  for (int i = 0; i < maxEnemyBullets; i++) {
    if (enemyBullets[i].active) {
      enemyBullets[i].y += 2;
      if (enemyBullets[i].y > SCREEN_HEIGHT) {
        enemyBullets[i].active = false;
      }

      // プレイヤーとの衝突判定
      if (enemyBullets[i].y >= playerY && enemyBullets[i].y <= playerY + playerHeight &&
          enemyBullets[i].x >= playerX && enemyBullets[i].x <= playerX + playerWidth) {
        gameOver = true;
      }
    }
  }

  // プレイヤーの弾と敵の衝突判定
  if (playerBullet.active) {
    for (int i = 0; i < enemyRows; i++) {
      for (int j = 0; j < enemyCols; j++) {
        if (enemies[i][j].alive) {
          if (playerBullet.x >= enemies[i][j].x &&
              playerBullet.x <= enemies[i][j].x + enemyWidth &&
              playerBullet.y >= enemies[i][j].y &&
              playerBullet.y <= enemies[i][j].y + enemyHeight) {
            enemies[i][j].alive = false;
            playerBullet.active = false;
            score += 10;
          }
        }
      }
    }
  }

  // 全滅チェック
  bool anyEnemyAlive = false;
  for (int i = 0; i < enemyRows; i++) {
    for (int j = 0; j < enemyCols; j++) {
      if (enemies[i][j].alive) {
        anyEnemyAlive = true;
        break;
      }
    }
    if (anyEnemyAlive) break;
  }
  if (!anyEnemyAlive) {
    gameWin = true;
  }

  // 描画
  display.clearDisplay();

  // 敵の描画
  for (int i = 0; i < enemyRows; i++) {
    for (int j = 0; j < enemyCols; j++) {
      if (enemies[i][j].alive) {
        drawEnemy((int)enemies[i][j].x, (int)enemies[i][j].y);
      }
    }
  }

  // プレイヤーの描画
  drawPlayer(playerX, playerY);

  // プレイヤーの弾の描画
  if (playerBullet.active) {
    display.fillRect((int)playerBullet.x, (int)playerBullet.y, 1, 3, SSD1306_WHITE);
  }

  // 敵の弾の描画
  for (int i = 0; i < maxEnemyBullets; i++) {
    if (enemyBullets[i].active) {
      display.fillRect((int)enemyBullets[i].x, (int)enemyBullets[i].y, 1, 3, SSD1306_WHITE);
    }
  }

  // スコア表示
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.print("S:");
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

void displayEndScreen() {
  display.clearDisplay();
  display.setTextSize(2);
  display.setTextColor(SSD1306_WHITE);

  if (gameWin) {
    display.setCursor(20, 15);
    display.print("YOU WIN!");
  } else {
    display.setCursor(10, 15);
    display.print("GAME OVER");
  }

  display.setTextSize(1);
  display.setCursor(30, 35);
  display.print("Score: ");
  display.print(score);

  display.setCursor(10, 50);
  display.print("LEFT to restart");

  display.display();
}
