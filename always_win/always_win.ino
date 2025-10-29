#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

const int LEFT_BUTTON_PIN = 5;   // スタート/リスタートボタン
const int RIGHT_BUTTON_PIN = 6;  // リール停止ボタン
const int BUZZER_PIN = 3;        // ブザー

// ゲーム状態
enum GameState {
  TITLE,
  READY,
  SPINNING,
  CELEBRATION,
  RESULT
};

GameState gameState = TITLE;

// スロットのシンボル
// 0=7, 1=BAR, 2=BELL, 3=STAR, 4=LEMON, 5=CHERRY
// 6=DIAMOND, 7=CROWN, 8=HEART, 9=CLOVER, 10=MOON, 11=SUN
// 12=APPLE, 13=GRAPE, 14=MELON, 15=ORANGE, 16=PEACH, 17=BANANA
const int numSymbols = 18;

// リールの状態
int reel1 = 0;
int reel2 = 0;
int reel3 = 0;

// リールの停止状態
bool reel1Stopped = false;
bool reel2Stopped = false;
bool reel3Stopped = false;

// スピン用アニメーション
unsigned long lastSpinUpdate = 0;
const int spinDelay = 100;      // スピン速度(ms)

unsigned long lastButtonPress = 0;
const int buttonDelay = 200;

// 演出用
unsigned long celebrationStartTime = 0;
int celebrationFrame = 0;

void setup() {
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
  pinMode(LEFT_BUTTON_PIN, INPUT_PULLUP);
  pinMode(RIGHT_BUTTON_PIN, INPUT_PULLUP);
  pinMode(BUZZER_PIN, OUTPUT);

  randomSeed(analogRead(0));  // 乱数の初期化

  // 初期リールをランダムに
  reel1 = random(numSymbols);
  reel2 = random(numSymbols);
  reel3 = random(numSymbols);

  showTitleScreen();
}

void resetGame() {
  reel1 = random(numSymbols);
  reel2 = random(numSymbols);
  reel3 = random(numSymbols);
  reel1Stopped = false;
  reel2Stopped = false;
  reel3Stopped = false;
  gameState = READY;
}

void loop() {
  unsigned long currentTime = millis();

  switch (gameState) {
    case TITLE:
      // 左ボタンでゲーム開始
      if (digitalRead(LEFT_BUTTON_PIN) == LOW &&
          currentTime - lastButtonPress > buttonDelay) {
        resetGame();
        lastButtonPress = currentTime;
        delay(50);
        while (digitalRead(LEFT_BUTTON_PIN) == LOW) delay(10);
      }
      break;

    case READY:
      drawReadyScreen();

      // 左ボタンでスピン開始
      if (digitalRead(LEFT_BUTTON_PIN) == LOW &&
          currentTime - lastButtonPress > buttonDelay) {
        gameState = SPINNING;
        lastSpinUpdate = currentTime;
        lastButtonPress = currentTime;
        delay(50);
        while (digitalRead(LEFT_BUTTON_PIN) == LOW) delay(10);
      }
      break;

    case SPINNING:
      // リールの回転処理
      if (currentTime - lastSpinUpdate > spinDelay) {
        if (!reel1Stopped) reel1 = (reel1 + 1) % numSymbols;
        if (!reel2Stopped) reel2 = (reel2 + 1) % numSymbols;
        if (!reel3Stopped) reel3 = (reel3 + 1) % numSymbols;
        lastSpinUpdate = currentTime;
      }

      // 右ボタンでリールを順番に停止
      if (digitalRead(RIGHT_BUTTON_PIN) == LOW &&
          currentTime - lastButtonPress > buttonDelay) {
        if (!reel1Stopped) {
          reel1Stopped = true;
          playStopSound();
        } else if (!reel2Stopped) {
          reel2Stopped = true;
          reel2 = reel1;  // 1個目と揃える
          playStopSound();
        } else if (!reel3Stopped) {
          reel3Stopped = true;
          reel3 = reel1;  // 1個目と揃える
          playStopSound();
          // 全リール停止したら演出開始
          gameState = CELEBRATION;
          celebrationStartTime = currentTime;
          celebrationFrame = 0;
        }
        lastButtonPress = currentTime;
        delay(50);
        while (digitalRead(RIGHT_BUTTON_PIN) == LOW) delay(10);
      }

      drawPlayingScreen();
      break;

    case CELEBRATION:
      // 演出アニメーション
      if (celebrationFrame == 0) {
        // 最初の1回だけファンファーレを鳴らす
        playCelebrationSound();
        celebrationFrame = 1;
      }

      // 演出中に追加の効果音
      unsigned long elapsed = currentTime - celebrationStartTime;
      int soundFrame = (elapsed / 150) % 3;
      if (elapsed > 100 && elapsed < 1900) {
        // 0.1秒後から1.9秒まで、150msごとに音を鳴らす
        static unsigned long lastSoundTime = 0;
        if (currentTime - lastSoundTime > 150) {
          tone(BUZZER_PIN, 880 + soundFrame * 220, 80);  // ラ、ド、ミの音階
          lastSoundTime = currentTime;
        }
      }

      drawCelebrationScreen(currentTime);

      // 2秒後に自動的に次のスピンへ
      if (currentTime - celebrationStartTime > 2000) {
        reel1Stopped = false;
        reel2Stopped = false;
        reel3Stopped = false;
        gameState = SPINNING;
        lastSpinUpdate = currentTime;
      }
      break;

    case RESULT:
      drawResultScreen();

      // 左ボタンですぐに次のスピン開始
      if (digitalRead(LEFT_BUTTON_PIN) == LOW &&
          currentTime - lastButtonPress > buttonDelay) {
        reel1Stopped = false;
        reel2Stopped = false;
        reel3Stopped = false;
        gameState = SPINNING;
        lastSpinUpdate = currentTime;
        lastButtonPress = currentTime;
        delay(50);
        while (digitalRead(LEFT_BUTTON_PIN) == LOW) delay(10);
      }
      break;
  }

  delay(20);
}

void showTitleScreen() {
  display.clearDisplay();
  display.setTextSize(2);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(15, 10);
  display.print("SLOT");
  display.setCursor(5, 28);
  display.print("MACHINE");

  display.setTextSize(1);
  display.setCursor(20, 50);
  display.print("L:START");

  display.display();
}

void drawReadyScreen() {
  display.clearDisplay();

  // リール枠とシンボル
  int reelY = 10;
  int reelWidth = 30;
  int reelHeight = 35;
  int reelSpacing = 4;
  int startX = 11;

  for (int i = 0; i < 3; i++) {
    int x = startX + i * (reelWidth + reelSpacing);
    display.drawRect(x, reelY, reelWidth, reelHeight, SSD1306_WHITE);
  }

  drawSymbol(startX + 3, reelY + 5, reel1);
  drawSymbol(startX + reelWidth + reelSpacing + 3, reelY + 5, reel2);
  drawSymbol(startX + (reelWidth + reelSpacing) * 2 + 3, reelY + 5, reel3);

  display.setTextSize(1);
  display.setCursor(30, 55);
  display.print("L:SPIN");

  display.display();
}

void drawPlayingScreen() {
  display.clearDisplay();

  // リール枠とシンボル
  int reelY = 10;
  int reelWidth = 30;
  int reelHeight = 35;
  int reelSpacing = 4;
  int startX = 11;

  for (int i = 0; i < 3; i++) {
    int x = startX + i * (reelWidth + reelSpacing);
    display.drawRect(x, reelY, reelWidth, reelHeight, SSD1306_WHITE);
  }

  drawSymbol(startX + 3, reelY + 5, reel1);
  drawSymbol(startX + reelWidth + reelSpacing + 3, reelY + 5, reel2);
  drawSymbol(startX + (reelWidth + reelSpacing) * 2 + 3, reelY + 5, reel3);

  // 停止マーク
  display.setTextSize(1);
  if (reel1Stopped) {
    display.setCursor(startX + 12, reelY + reelHeight + 2);
    display.print("*");
  }
  if (reel2Stopped) {
    display.setCursor(startX + reelWidth + reelSpacing + 12, reelY + reelHeight + 2);
    display.print("*");
  }
  if (reel3Stopped) {
    display.setCursor(startX + (reelWidth + reelSpacing) * 2 + 12, reelY + reelHeight + 2);
    display.print("*");
  }

  display.setCursor(28, 55);
  display.print("R:STOP");

  display.display();
}

void drawResultScreen() {
  display.clearDisplay();

  // リール結果
  int reelY = 10;
  int reelWidth = 30;
  int reelHeight = 35;
  int reelSpacing = 4;
  int startX = 11;

  for (int i = 0; i < 3; i++) {
    int x = startX + i * (reelWidth + reelSpacing);
    display.drawRect(x, reelY, reelWidth, reelHeight, SSD1306_WHITE);
  }

  drawSymbol(startX + 3, reelY + 5, reel1);
  drawSymbol(startX + reelWidth + reelSpacing + 3, reelY + 5, reel2);
  drawSymbol(startX + (reelWidth + reelSpacing) * 2 + 3, reelY + 5, reel3);

  // 次へのメッセージ
  display.setTextSize(1);
  display.setCursor(30, 55);
  display.print("L:SPIN");

  display.display();
}

// ASCIIアートでシンボルを描画
void drawSymbol(int x, int y, int symbolType) {
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);

  switch (symbolType) {
    case 0:  // 7
      display.setCursor(x + 8, y + 10);
      display.setTextSize(2);
      display.print("7");
      display.setTextSize(1);
      break;

    case 1:  // BAR
      display.setCursor(x + 4, y + 10);
      display.print("BAR");
      break;

    case 2:  // BELL (ベル)
      // ベルの上部
      display.fillRect(x + 10, y + 3, 4, 2, SSD1306_WHITE);
      // ベルの本体
      display.drawLine(x + 6, y + 5, x + 12, y + 16, SSD1306_WHITE);
      display.drawLine(x + 17, y + 5, x + 12, y + 16, SSD1306_WHITE);
      display.drawLine(x + 6, y + 5, x + 17, y + 5, SSD1306_WHITE);
      display.drawLine(x + 5, y + 16, x + 19, y + 16, SSD1306_WHITE);
      // ベルの舌
      display.fillRect(x + 11, y + 17, 2, 3, SSD1306_WHITE);
      break;

    case 3:  // STAR (星)
      // 5芒星
      display.drawLine(x + 12, y + 2, x + 14, y + 10, SSD1306_WHITE);  // 上→右下
      display.drawLine(x + 14, y + 10, x + 6, y + 6, SSD1306_WHITE);   // 右下→左
      display.drawLine(x + 6, y + 6, x + 18, y + 6, SSD1306_WHITE);    // 左→右
      display.drawLine(x + 18, y + 6, x + 10, y + 10, SSD1306_WHITE);  // 右→左下
      display.drawLine(x + 10, y + 10, x + 12, y + 2, SSD1306_WHITE);  // 左下→上
      break;

    case 4:  // LEMON (レモン)
      // レモンの輪郭
      display.drawCircle(x + 12, y + 10, 8, SSD1306_WHITE);
      // レモンの先端
      display.fillTriangle(x + 18, y + 6, x + 20, y + 8, x + 18, y + 10, SSD1306_WHITE);
      display.fillTriangle(x + 6, y + 10, x + 4, y + 8, x + 6, y + 6, SSD1306_WHITE);
      // 内側の線
      display.drawLine(x + 12, y + 4, x + 12, y + 16, SSD1306_WHITE);
      break;

    case 5:  // CHERRY (チェリー)
      // 2つのさくらんぼ
      display.fillCircle(x + 8, y + 13, 4, SSD1306_WHITE);
      display.fillCircle(x + 16, y + 13, 4, SSD1306_WHITE);
      // 軸
      display.drawLine(x + 8, y + 9, x + 12, y + 3, SSD1306_WHITE);
      display.drawLine(x + 16, y + 9, x + 12, y + 3, SSD1306_WHITE);
      // 葉っぱ
      display.drawLine(x + 12, y + 3, x + 16, y + 2, SSD1306_WHITE);
      display.drawLine(x + 16, y + 2, x + 15, y + 5, SSD1306_WHITE);
      break;

    case 6:  // DIAMOND (ダイヤモンド)
      display.drawLine(x + 12, y + 2, x + 18, y + 8, SSD1306_WHITE);
      display.drawLine(x + 18, y + 8, x + 12, y + 18, SSD1306_WHITE);
      display.drawLine(x + 12, y + 18, x + 6, y + 8, SSD1306_WHITE);
      display.drawLine(x + 6, y + 8, x + 12, y + 2, SSD1306_WHITE);
      display.drawLine(x + 6, y + 8, x + 18, y + 8, SSD1306_WHITE);
      break;

    case 7:  // CROWN (王冠)
      display.drawLine(x + 5, y + 15, x + 19, y + 15, SSD1306_WHITE);
      display.drawLine(x + 5, y + 15, x + 6, y + 8, SSD1306_WHITE);
      display.drawLine(x + 6, y + 8, x + 8, y + 12, SSD1306_WHITE);
      display.drawLine(x + 8, y + 12, x + 12, y + 5, SSD1306_WHITE);
      display.drawLine(x + 12, y + 5, x + 16, y + 12, SSD1306_WHITE);
      display.drawLine(x + 16, y + 12, x + 18, y + 8, SSD1306_WHITE);
      display.drawLine(x + 18, y + 8, x + 19, y + 15, SSD1306_WHITE);
      break;

    case 8:  // HEART (ハート)
      display.fillCircle(x + 8, y + 7, 3, SSD1306_WHITE);
      display.fillCircle(x + 16, y + 7, 3, SSD1306_WHITE);
      display.fillTriangle(x + 5, y + 8, x + 19, y + 8, x + 12, y + 18, SSD1306_WHITE);
      break;

    case 9:  // CLOVER (クローバー)
      display.fillCircle(x + 12, y + 6, 3, SSD1306_WHITE);
      display.fillCircle(x + 9, y + 10, 3, SSD1306_WHITE);
      display.fillCircle(x + 15, y + 10, 3, SSD1306_WHITE);
      display.fillCircle(x + 12, y + 14, 3, SSD1306_WHITE);
      display.drawLine(x + 12, y + 14, x + 12, y + 18, SSD1306_WHITE);
      break;

    case 10:  // MOON (月)
      display.fillCircle(x + 12, y + 10, 6, SSD1306_WHITE);
      display.fillCircle(x + 15, y + 10, 6, SSD1306_BLACK);
      break;

    case 11:  // SUN (太陽)
      display.fillCircle(x + 12, y + 10, 4, SSD1306_WHITE);
      // 光線
      for (int i = 0; i < 8; i++) {
        float angle = i * 3.14159 / 4;
        int x1 = x + 12 + cos(angle) * 6;
        int y1 = y + 10 + sin(angle) * 6;
        int x2 = x + 12 + cos(angle) * 9;
        int y2 = y + 10 + sin(angle) * 9;
        display.drawLine(x1, y1, x2, y2, SSD1306_WHITE);
      }
      break;

    case 12:  // APPLE (リンゴ)
      display.fillCircle(x + 12, y + 11, 6, SSD1306_WHITE);
      display.drawLine(x + 12, y + 5, x + 12, y + 8, SSD1306_WHITE);
      display.drawLine(x + 12, y + 5, x + 14, y + 6, SSD1306_WHITE);
      break;

    case 13:  // GRAPE (ブドウ)
      display.fillCircle(x + 12, y + 6, 3, SSD1306_WHITE);
      display.fillCircle(x + 10, y + 9, 3, SSD1306_WHITE);
      display.fillCircle(x + 14, y + 9, 3, SSD1306_WHITE);
      display.fillCircle(x + 9, y + 12, 3, SSD1306_WHITE);
      display.fillCircle(x + 12, y + 12, 3, SSD1306_WHITE);
      display.fillCircle(x + 15, y + 12, 3, SSD1306_WHITE);
      display.fillCircle(x + 11, y + 15, 3, SSD1306_WHITE);
      display.fillCircle(x + 13, y + 15, 3, SSD1306_WHITE);
      break;

    case 14:  // MELON (メロン)
      display.fillCircle(x + 12, y + 11, 7, SSD1306_WHITE);
      display.drawLine(x + 12, y + 4, x + 9, y + 7, SSD1306_WHITE);
      display.drawLine(x + 12, y + 4, x + 15, y + 7, SSD1306_WHITE);
      display.drawLine(x + 12, y + 7, x + 12, y + 14, SSD1306_BLACK);
      display.drawLine(x + 9, y + 9, x + 15, y + 9, SSD1306_BLACK);
      break;

    case 15:  // ORANGE (オレンジ)
      display.fillCircle(x + 12, y + 11, 6, SSD1306_WHITE);
      display.drawLine(x + 12, y + 5, x + 12, y + 8, SSD1306_WHITE);
      display.fillRect(x + 11, y + 4, 2, 2, SSD1306_WHITE);
      break;

    case 16:  // PEACH (桃)
      display.fillCircle(x + 10, y + 11, 5, SSD1306_WHITE);
      display.fillCircle(x + 14, y + 11, 5, SSD1306_WHITE);
      display.fillTriangle(x + 12, y + 6, x + 7, y + 12, x + 17, y + 12, SSD1306_WHITE);
      display.drawLine(x + 12, y + 3, x + 12, y + 6, SSD1306_WHITE);
      break;

    case 17:  // BANANA (バナナ)
      display.drawLine(x + 8, y + 6, x + 10, y + 4, SSD1306_WHITE);
      display.drawLine(x + 10, y + 4, x + 14, y + 5, SSD1306_WHITE);
      display.drawLine(x + 14, y + 5, x + 16, y + 8, SSD1306_WHITE);
      display.drawLine(x + 16, y + 8, x + 16, y + 12, SSD1306_WHITE);
      display.drawLine(x + 16, y + 12, x + 14, y + 16, SSD1306_WHITE);
      display.drawLine(x + 14, y + 16, x + 10, y + 16, SSD1306_WHITE);
      display.drawLine(x + 10, y + 16, x + 8, y + 13, SSD1306_WHITE);
      display.drawLine(x + 8, y + 13, x + 8, y + 6, SSD1306_WHITE);
      break;
  }
}

// リール停止時の効果音
void playStopSound() {
  tone(BUZZER_PIN, 800, 100);  // 800Hzの音を100ms鳴らす
}

// 派手な演出の描画
void drawCelebrationScreen(unsigned long currentTime) {
  display.clearDisplay();

  unsigned long elapsed = currentTime - celebrationStartTime;
  int frame = (elapsed / 100) % 10;  // 100msごとにフレーム更新

  // リール枠とシンボル
  int reelY = 10;
  int reelWidth = 30;
  int reelHeight = 35;
  int reelSpacing = 4;
  int startX = 11;

  // 点滅効果（偶数フレームで枠を太く表示）
  for (int i = 0; i < 3; i++) {
    int x = startX + i * (reelWidth + reelSpacing);
    if (frame % 2 == 0) {
      // 太枠
      display.drawRect(x, reelY, reelWidth, reelHeight, SSD1306_WHITE);
      display.drawRect(x + 1, reelY + 1, reelWidth - 2, reelHeight - 2, SSD1306_WHITE);
    } else {
      display.drawRect(x, reelY, reelWidth, reelHeight, SSD1306_WHITE);
    }
  }

  drawSymbol(startX + 3, reelY + 5, reel1);
  drawSymbol(startX + reelWidth + reelSpacing + 3, reelY + 5, reel2);
  drawSymbol(startX + (reelWidth + reelSpacing) * 2 + 3, reelY + 5, reel3);

  // WIN! 文字を点滅
  if (frame % 2 == 0) {
    display.setTextSize(2);
    display.setCursor(35, 50);
    display.print("WIN!");
  }

  // 周りに星を描画（回転アニメーション）
  int starPositions[][2] = {
    {10, 5}, {60, 3}, {110, 5},
    {5, 30}, {115, 30},
    {10, 55}, {110, 55}
  };

  for (int i = 0; i < 7; i++) {
    if ((frame + i) % 3 == 0) {
      int sx = starPositions[i][0];
      int sy = starPositions[i][1];
      // 小さい星
      display.drawPixel(sx, sy, SSD1306_WHITE);
      display.drawPixel(sx - 1, sy, SSD1306_WHITE);
      display.drawPixel(sx + 1, sy, SSD1306_WHITE);
      display.drawPixel(sx, sy - 1, SSD1306_WHITE);
      display.drawPixel(sx, sy + 1, SSD1306_WHITE);
    }
  }

  display.display();
}

// 派手な祝福サウンド
void playCelebrationSound() {
  // 最初の高い音を鳴らす
  tone(BUZZER_PIN, 1047, 500);  // 高いドを500ms鳴らす
}
