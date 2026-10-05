# CLAUDE.md

## 專案
五子棋（Gomoku），C++20 + Qt 6。
- 對戰選擇：人對人（區網連線）、人對電腦（三種難度）、電腦對電腦。
- 模式選擇：正常對戰模式、限時模式（時間需自行設定）
- 功能：存/讀檔、悔棋、棋譜紀錄與回放、AI 提示。
  完整設計在 `docs/SDD.md`，做較大的改動前先讀它；SDD 與程式碼衝突時，先問我，不要自己決定。

## 開發環境
- Debian 13（trixie）、g++ 14、Qt 6.8（apt 套件 `qt6-base-dev`）、CMake、Ninja
- GoogleTest 由 CMake `FetchContent` 自動下載，不使用系統套件
- CI（GitHub Actions）在 `debian:trixie` 容器內執行，用與本機相同的 apt 套件安裝依賴，確保環境一致；不要改用其他 Qt 安裝方式

## 常用指令
```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug   # 設定
cmake --build build -j                          # 編譯
ctest --test-dir build --output-on-failure      # 跑全部測試
./build/gomoku                                  # 執行
```

## 目錄結構
```
src/core/   Board, Rules, MoveHistory, GameState, GameClock, GameRecord, ReplayCursor —— 純 C++，禁止 include 任何 Qt 標頭
src/ai/     Minimax + Alpha-Beta、評估函數、候選步產生 —— 只依賴 core
src/format/ SaveFormat：存檔／棋譜與 JSON 互轉 —— 只依賴 core 與 Qt6::Core
src/net/    QTcpServer / QTcpSocket 連線與訊息協定
src/ui/     Qt Widgets、BoardView（QPainter 繪製）、對話框
src/app/    GameController：串起 core / ai / net / ui，管理回合與遊戲狀態
tests/      GoogleTest，資料夾結構對應 src/
docs/       SDD.md、棋譜格式說明
```
依賴方向只能往下：ui / net / app → ai / format → core。core 不可反向依賴任何上層；format 不可依賴 ai / net / ui / app。

## 遊戲規則（固定，改規則要先改 SDD）
- 棋盤 15×15，座標 (row, col)，皆從 0 開始，(0,0) 在左上角
- 黑先，輪流落子
- 連成五子（含）以上即勝，無禁手
- 棋盤下滿無人連五為和局
- 限時模式的計時方式與超時判定見 `docs/SDD.md`

## 實作慣例
- AI 搜尋一律在背景執行緒（QThread / worker object），不得阻塞 UI 執行緒；結果用 signal 傳回
- AI 必須支援「最大深度」與「時間上限」兩種停止條件，且可被中途取消（悔棋、離開對局、超時時）
- 限時模式下，AI 的思考時間上限不得超過該方剩餘時間（需預留緩衝）
- AI 給定相同局面與相同參數時結果必須可重現（測試依賴這點，不要用未固定種子的亂數）
- GameClock 放在 core，不可直接使用 QTimer 或系統時間；時間來源以介面注入，測試時用假時鐘控制，不可在測試中 sleep
- 連線：開房的一方是 server，且是唯一的權威狀態（含計時）；client 送出落子請求，server 驗證後廣播，client 只負責顯示時間
- 存檔與棋譜格式定義在 `docs/SDD.md`（限時模式需包含時間設定與雙方剩餘時間），修改格式需同步更新文件與測試
- 命名：類別 PascalCase、函式與變數 camelCase、成員變數加 `m_` 前綴
- 不新增第三方函式庫，除非先問過我

## 工作方式
- 一次只做一個小步驟，做完就停下來說明改了什麼
- core 與 ai 的功能先寫測試再實作；改完一定要跑 `ctest`，全部通過才算完成
- 不可為了讓測試通過而修改或刪除既有測試，測試有問題要先告訴我
- 修 bug 時先寫一個能重現 bug 的失敗測試
- 不確定需求時直接問，不要猜