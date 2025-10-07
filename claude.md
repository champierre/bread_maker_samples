# Arduino でゲームを作る際の注意点

## 1. ディスプレイの初期化と動作確認

### 問題
- `display.begin()` が失敗しても、エラーチェックなしだと画面に何も表示されない
- 複雑なコードでは問題の切り分けが難しい

### 解決策
- **最初に最小限のコードでディスプレイ表示を確認する**
- タイトル画面だけを表示するシンプルなコードから始める
- 表示が確認できてから、段階的に機能を追加する

```cpp
void setup() {
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
  display.clearDisplay();
  display.setTextSize(2);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(10, 20);
  display.print("Game Title");
  display.display();  // これを忘れると何も表示されない
}
```

## 2. display.display() の呼び出し

### 重要
- Adafruit_SSD1306 では、描画コマンド後に **必ず `display.display()` を呼ぶ**
- これを呼ばないと、画面に反映されない

## 3. ボタン入力のデバッグ

### 問題
- ボタンが反応しない原因の切り分けが難しい
  - 配線の問題？
  - ピン番号の問題？
  - プルアップの問題？
  - コードのロジックの問題？

### 解決策
- **ボタンの状態を画面に常時表示する**
- デバッグモードで `digitalRead()` の値を表示

```cpp
void loop() {
  display.setCursor(0, 0);
  display.print("L:");
  display.print(digitalRead(LEFT_BUTTON_PIN));
  display.print(" R:");
  display.print(digitalRead(RIGHT_BUTTON_PIN));
  display.display();
  delay(100);
}
```

## 4. 段階的な実装

### 重要
- **いきなり完全なゲームコードを書かない**
- 各機能を小さいステップに分けて実装
- 各ステップで動作確認してから次に進む

### 推奨アプローチ
1. タイトル画面の表示
2. ボタン入力の確認
3. ゲーム開始の処理
4. 基本的なオブジェクト（プレイヤー、敵など）の描画
5. 移動処理
6. 衝突判定
7. スコア表示
8. ゲームオーバー処理

## 5. メモリ制限

### 注意
- Arduino は RAM が非常に限られている（Uno: 2KB）
- 大きな配列は避ける
- String クラスより char 配列を使う

### 例
```cpp
// 良い例
const int maxLength = 200;
int snakeX[maxLength];

// 悪い例（メモリを圧迫）
int snakeX[1000];
```

## 6. タイミングとディレイ

### 注意
- `delay()` は処理を完全に止めてしまう
- ボタン入力が反応しなくなる可能性

### 推奨
- `millis()` を使った非ブロッキングなタイミング制御

```cpp
unsigned long lastMoveTime = 0;
int moveDelay = 150;

void loop() {
  unsigned long currentTime = millis();

  if (currentTime - lastMoveTime > moveDelay) {
    // 移動処理
    lastMoveTime = currentTime;
  }
}
```

## 7. 動作するコードをベースにする

### 重要
- すでに動作しているコード（invador.ino など）を参考にする
- 同じ初期化方法、同じライブラリの使い方を踏襲する
- 動作確認済みのコード構造を真似る

## 8. デバッグの基本

### トラブルシューティング手順
1. 最小限のコードで動作確認（タイトル表示のみ）
2. ボタン入力の確認（状態を画面表示）
3. 段階的に機能を追加
4. 各追加ごとに動作確認

### 問題が起きたら
- 最後に動いていた状態まで戻す
- 1つずつ機能を追加して、どこで問題が起きるか特定する

## 9. I2C アドレスの確認

### 問題
- OLED ディスプレイの I2C アドレスが 0x3C または 0x3D
- 間違ったアドレスだと何も表示されない

### 解決策
- I2C スキャナーで正しいアドレスを確認
- 動作しているコードと同じアドレスを使う

## 10. ゲームループの基本構造

### 推奨構造
```cpp
void loop() {
  // 1. ゲーム状態の確認
  if (!gameStarted) {
    // タイトル画面処理
    return;
  }

  if (gameOver) {
    // ゲームオーバー処理
    return;
  }

  // 2. 入力処理
  // ボタン読み取り

  // 3. ゲームロジック
  // 移動、衝突判定など

  // 4. 描画
  display.clearDisplay();
  // 描画処理
  display.display();

  // 5. フレームレート調整
  delay(10);
}
```

## まとめ

Arduino でゲームを作る際は：
1. **最小限のコードから始める**
2. **段階的に機能を追加**
3. **各ステップで動作確認**
4. **ボタンやセンサーの状態を視覚化してデバッグ**
5. **動作するコードをベースにする**

これらを守ることで、トラブルシューティングが容易になり、確実に動作するゲームが作れます。
