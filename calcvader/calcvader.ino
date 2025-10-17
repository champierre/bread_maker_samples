/*
 * CALCVADER - 電卓インベーダーゲーム
 * CASIO SL-880のインベーダーゲームを再現
 */

#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// ディスプレイ設定
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// ボタンピン設定
#define AIM_BUTTON_PIN 5
#define FIRE_BUTTON_PIN 6

// ゲーム定数
const int MAX_VISIBLE_PART1 = 6;
const int MAX_VISIBLE_PART2 = 5;
const int TOTAL_INVADERS_PER_ROUND = 16;
const int MAX_AMMO = 30;
const int UFO_BONUS = 300;

// インベーダー構造体
struct Invader {
  int number;        // 数字（0-9）
  int position;      // 位置（1～6の桁数）
  int x, y;          // 座標
  bool active;       // 生存フラグ
  bool isUFO;        // UFOフラグ
};

Invader invaders[6];

// ゲーム状態変数
int aim = 0;                    // 照準（0-9, n=10）
int sum = 0;                    // 撃破した数字の合計
bool ufoActive = false;         // UFO出現フラグ
int ufoIndex = -1;              // UFOのインデックス
int score = 0;
int life = 3;                   // ライフ
int ammo = MAX_AMMO;
int pattern = 1;                // 現在のパターン（1～9）
int part = 1;                   // 現在のパート（1 or 2）
int invadersDefeated = 0;       // 撃破したインベーダー数
int invadersSpawned = 0;        // 出現したインベーダー数

// ゲーム状態
enum GameState {
  TITLE,
  PLAYING,
  GAME_OVER
};
GameState gameState = TITLE;

// ボタン状態
bool lastAimButton = HIGH;
bool lastFireButton = HIGH;

// タイミング制御
unsigned long lastMoveTime = 0;
int moveDelay = 1500;            // インベーダーの移動間隔（ms）

void setup() {
  Serial.begin(9600);

  // ボタン設定
  pinMode(AIM_BUTTON_PIN, INPUT_PULLUP);
  pinMode(FIRE_BUTTON_PIN, INPUT_PULLUP);

  // ディスプレイ初期化
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println(F("SSD1306 allocation failed"));
    for (;;);
  }

  display.clearDisplay();
  display.display();

  randomSeed(analogRead(0));
}

void loop() {
  switch (gameState) {
    case TITLE:
      updateTitle();
      break;
    case PLAYING:
      updateGame();
      break;
    case GAME_OVER:
      updateGameOver();
      break;
  }
}

// タイトル画面の更新
void updateTitle() {
  display.clearDisplay();
  display.setTextSize(2);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(10, 10);
  display.println(F("CALCVADER"));

  display.setTextSize(1);
  display.setCursor(10, 35);
  display.println(F("Press FIRE"));
  display.setCursor(10, 45);
  display.println(F("to start"));

  display.display();

  // FIREボタンでゲーム開始
  bool fireButton = digitalRead(FIRE_BUTTON_PIN);
  if (lastFireButton == HIGH && fireButton == LOW) {
    initGame();
    gameState = PLAYING;
  }
  lastFireButton = fireButton;
}

// ゲーム初期化
void initGame() {
  aim = 0;
  sum = 0;
  ufoActive = false;
  ufoIndex = -1;
  score = 0;
  life = 3;
  ammo = MAX_AMMO;
  pattern = 1;
  part = 1;
  invadersDefeated = 0;
  invadersSpawned = 0;

  // インベーダー初期化
  for (int i = 0; i < 6; i++) {
    invaders[i].active = false;
  }

  spawnInitialInvaders();

  lastMoveTime = millis();
  moveDelay = 1500;
}

// 初期インベーダーを配置
void spawnInitialInvaders() {
  int startPos = (part == 1) ? 6 : 5;

  // 最初は1個だけ配置
  if (invadersSpawned < TOTAL_INVADERS_PER_ROUND) {
    spawnInvader(0, startPos);
  }
}

// インベーダーを生成
void spawnInvader(int index, int position) {
  invaders[index].number = random(0, 10);
  invaders[index].position = position;
  invaders[index].x = 60 + (position - 1) * 12;
  invaders[index].y = 28;
  invaders[index].active = true;
  invaders[index].isUFO = false;
  invadersSpawned++;
}

// ゲーム画面の更新
void updateGame() {
  unsigned long currentTime = millis();

  // ボタン入力処理
  handleInput();

  // インベーダー移動
  if (currentTime - lastMoveTime > moveDelay) {
    moveInvaders();
    lastMoveTime = currentTime;
  }

  // 描画
  drawGame();

  // ゲームオーバーチェック
  if (life <= 0 || ammo <= 0) {
    gameState = GAME_OVER;
  }

  // ラウンドクリアチェック
  if (invadersDefeated >= TOTAL_INVADERS_PER_ROUND) {
    nextPattern();
  }
}

// ボタン入力処理
void handleInput() {
  // AIMボタン
  bool aimButton = digitalRead(AIM_BUTTON_PIN);
  if (lastAimButton == HIGH && aimButton == LOW) {
    aim++;
    if (aim > 10) aim = 0;  // 0-9, n(10)
  }
  lastAimButton = aimButton;

  // FIREボタン
  bool fireButton = digitalRead(FIRE_BUTTON_PIN);
  if (lastFireButton == HIGH && fireButton == LOW) {
    fire();
  }
  lastFireButton = fireButton;
}

// 撃破されたインベーダーより左にいるものを右に詰める
void shiftInvadersRight(int defeatedPosition) {
  // 撃破されたインベーダーより左（positionが小さい）にいるすべてのインベーダーを右に1つ移動
  for (int i = 0; i < 6; i++) {
    if (invaders[i].active && invaders[i].position < defeatedPosition) {
      invaders[i].position++;
      invaders[i].x += 12;  // X座標も右に移動
    }
  }
}

// 配列を詰める処理（撃破された穴を埋める）
void compactInvaders() {
  // アクティブなインベーダーを前に詰める
  int writeIdx = 0;
  for (int i = 0; i < 6; i++) {
    if (invaders[i].active) {
      if (i != writeIdx) {
        invaders[writeIdx] = invaders[i];

        // UFOが移動した場合、インデックスを更新
        if (invaders[writeIdx].isUFO) {
          ufoIndex = writeIdx;
        }
      }
      writeIdx++;
    }
  }

  // 詰めた後の空きスロットをクリア
  for (int i = writeIdx; i < 6; i++) {
    invaders[i].active = false;
    invaders[i].number = 0;
    invaders[i].isUFO = false;
  }

  // X座標を再計算（右詰めで表示するため、位置に基づいてX座標を設定）
  for (int i = 0; i < writeIdx; i++) {
    invaders[i].x = 60 + (invaders[i].position - 1) * 12;
  }
}

// 発射処理
void fire() {
  if (ammo <= 0) return;

  ammo--;

  // UFO狙い
  if (aim == 10 && ufoActive && ufoIndex >= 0) {
    if (invaders[ufoIndex].active && invaders[ufoIndex].isUFO) {
      int defeatedPosition = invaders[ufoIndex].position;
      invaders[ufoIndex].active = false;
      score += UFO_BONUS;
      ufoActive = false;
      ufoIndex = -1;
      invadersDefeated++;

      // 撃破されたインベーダーより左にいるものを右に詰める
      shiftInvadersRight(defeatedPosition);

      // 配列を詰める
      compactInvaders();
    }
    return;
  }

  // 通常のインベーダー狙い
  for (int i = 0; i < 6; i++) {
    if (invaders[i].active && !invaders[i].isUFO && invaders[i].number == aim) {
      int defeatedPosition = invaders[i].position;
      invaders[i].active = false;

      // 得点計算（位置に応じた得点）
      int multiplier = (part == 1) ? 10 : 20;
      score += invaders[i].position * multiplier;

      // SUMに加算
      sum += invaders[i].number;

      invadersDefeated++;

      // 撃破されたインベーダーより左にいるものを右に詰める
      shiftInvadersRight(defeatedPosition);

      // 配列を詰める
      compactInvaders();

      // UFO出現チェック
      if (sum % 10 == 0 && !ufoActive) {
        spawnUFO();
      }

      break;
    }
  }
}

// UFOを生成
void spawnUFO() {
  // 空いているスロットを探す
  for (int i = 0; i < 6; i++) {
    if (!invaders[i].active) {
      int startPos = (part == 1) ? 6 : 5;
      invaders[i].number = 10;  // nを表す
      invaders[i].position = startPos;
      invaders[i].x = 60 + (startPos - 1) * 12;
      invaders[i].y = 28;
      invaders[i].active = true;
      invaders[i].isUFO = true;
      ufoActive = true;
      ufoIndex = i;
      break;
    }
  }
}

// インベーダーを移動
void moveInvaders() {
  bool needCompact = false;

  for (int i = 0; i < 6; i++) {
    if (invaders[i].active) {
      invaders[i].x -= 12;
      invaders[i].position--;

      // 左端に到達
      if (invaders[i].position <= 0) {
        invaders[i].active = false;
        needCompact = true;

        if (invaders[i].isUFO) {
          ufoActive = false;
          ufoIndex = -1;
        } else {
          life--;  // ライフ減少
        }

        // 左端到達もカウント（撃破ではないが処理済み）
        invadersDefeated++;
      }
    }
  }

  // 左端到達で消えたインベーダーがいたら配列を詰める
  if (needCompact) {
    compactInvaders();
  }

  // 新しいインベーダーを生成（左端到達分を補充）
  if (needCompact && invadersSpawned < TOTAL_INVADERS_PER_ROUND) {
    for (int i = 0; i < 6; i++) {
      if (!invaders[i].active) {
        int startPos = (part == 1) ? 6 : 5;
        spawnInvader(i, startPos);
        break;
      }
    }
  }

  // 画面上のアクティブなインベーダーをカウント
  int activeCount = 0;
  for (int i = 0; i < 6; i++) {
    if (invaders[i].active) {
      activeCount++;
    }
  }

  // 最大数未満なら新しいインベーダーを追加
  int maxVisible = (part == 1) ? MAX_VISIBLE_PART1 : MAX_VISIBLE_PART2;
  if (activeCount < maxVisible && invadersSpawned < TOTAL_INVADERS_PER_ROUND) {
    for (int i = 0; i < 6; i++) {
      if (!invaders[i].active) {
        int startPos = (part == 1) ? 6 : 5;
        spawnInvader(i, startPos);
        break;
      }
    }
  }
}

// 次のパターンへ
void nextPattern() {
  // ラウンドクリア表示
  display.clearDisplay();
  display.setTextSize(2);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(10, 20);
  display.println(F("ROUND"));
  display.setCursor(10, 40);
  display.println(F("CLEAR!"));
  display.display();

  // インターバル（2秒）
  delay(2000);

  pattern++;
  if (pattern > 9) {
    pattern = 1;
    part++;
    if (part > 2) {
      part = 1;
    }
  }

  // 速度アップ
  moveDelay = max(600, 1500 - (pattern - 1) * 100);

  // ラウンドリセット
  invadersDefeated = 0;
  invadersSpawned = 0;
  sum = 0;
  ammo = MAX_AMMO;
  ufoActive = false;
  ufoIndex = -1;

  for (int i = 0; i < 6; i++) {
    invaders[i].active = false;
  }

  spawnInitialInvaders();

  // タイマーをリセット
  lastMoveTime = millis();
}

// ゲーム画面描画
void drawGame() {
  display.clearDisplay();

  // 照準を描画（左端、大きく）
  drawLargeNumber(aim, 5, 25);

  // 残機を描画（照準の右隣）
  drawLife(25, 30);

  // インベーダーを描画
  drawInvaders();

  // 下部情報
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 56);
  display.print(F("SUM:"));
  display.print(sum);
  display.setCursor(70, 56);
  display.print(F("AMMO:"));
  display.print(ammo);

  // スコア（上部）
  display.setCursor(0, 0);
  display.print(F("SC:"));
  display.print(score);

  // パート・パターン（上部右）
  display.setCursor(90, 0);
  display.print(part);
  display.print(F("-"));
  display.print(pattern);

  display.display();
}

// 大きな数字を描画（7セグメント風）
void drawLargeNumber(int num, int x, int y) {
  display.setTextSize(2);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(x, y);

  if (num == 10) {
    display.print(F("n"));
  } else {
    display.print(num);
  }
}

// 残機を描画
void drawLife(int x, int y) {
  display.setTextSize(2);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(x, y);

  if (life == 3) {
    display.print(F("E"));
  } else if (life == 2) {
    display.print(F("="));
  } else if (life == 1) {
    display.print(F("-"));
  }
  // life == 0 は何も表示しない
}

// インベーダーを描画
void drawInvaders() {
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);

  // アクティブなインベーダーを位置順にソートして描画（右詰め）
  // 位置が大きい順（右から左）にソートする
  for (int pos = 6; pos >= 1; pos--) {
    for (int i = 0; i < 6; i++) {
      if (invaders[i].active && invaders[i].position == pos) {
        display.setCursor(invaders[i].x, invaders[i].y);

        // UFOの場合は'n'を表示
        if (invaders[i].isUFO) {
          display.print(F("n"));
        }
        // 通常のインベーダー: 0-9の範囲チェック
        else if (invaders[i].number >= 0 && invaders[i].number <= 9) {
          display.print(invaders[i].number);
        }
        // それ以外は表示しない（不正な値）
      }
    }
  }
}

// ゲームオーバー画面の更新
void updateGameOver() {
  display.clearDisplay();
  display.setTextSize(2);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(10, 10);
  display.println(F("GAME OVER"));

  display.setTextSize(1);
  display.setCursor(10, 35);
  display.print(F("Score: "));
  display.println(score);

  display.setCursor(0, 50);
  display.println(F("Press both to restart"));

  display.display();

  // 両方のボタンが同時押しされたらタイトルへ
  if (digitalRead(AIM_BUTTON_PIN) == LOW && digitalRead(FIRE_BUTTON_PIN) == LOW) {
    delay(50);
    // ボタンが離されるまで待つ
    while (digitalRead(AIM_BUTTON_PIN) == LOW || digitalRead(FIRE_BUTTON_PIN) == LOW) {
      delay(10);
    }
    gameState = TITLE;
  }
}
