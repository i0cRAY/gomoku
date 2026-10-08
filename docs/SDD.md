# 五子棋 Gomoku — 軟體設計文件（SDD）

| 項目 | 內容 |
|---|---|
| 作者 | 林希叡 |
| 版本 | 0.4（草稿） |
| 日期 | 2026-10-05 |
| 語言／框架 | C++20、Qt 6（Widgets、Network）、GoogleTest、CMake |

---

## 1. 簡介

### 1.1 目的
一款桌面五子棋遊戲，支援人對電腦、電腦對電腦與區網連線對戰，提供正常與限時兩種模式，以及存讀檔、悔棋、棋譜回放、AI 提示等功能。本專案特別著重 AI 棋力。

### 1.2 範圍
**這版要做：**
- 對戰類型：人對人（區網連線）、人對人（本機雙人）、人對電腦（簡單／普通／困難）、電腦對電腦
- 模式：正常對戰、限時（每步限時，秒數可自訂）
- 功能：存檔／讀檔、悔棋、棋譜紀錄與回放、AI 提示

**這版不做：**
- 禁手規則（Renju）
- 跨網際網路連線（NAT 穿透）、線上配對、帳號系統
- 音效、主題換膚

### 1.3 名詞
| 名詞 | 意義 |
|---|---|
| 步（ply） | 一方落一子 |
| 局面 | 某一時刻棋盤上所有棋子的狀態 |
| 活四／衝四／活三… | 棋型，定義見 §5.3.2 |
| Host | 區網連線中開房的一方，同時擔任 server |
| Guest | 區網連線中加入的一方，擔任 client |

---

## 2. 需求

### 2.1 對戰類型
| 類型 | 黑方 | 白方 | 備註 |
|---|---|---|---|
| 人對電腦 | 玩家可選執黑或執白 | 另一方為 AI | AI 難度三選一 |
| 電腦對電腦 | AI | AI | 雙方難度可分別設定；每步間隔 500 ms 方便觀看 |
| 區網連線 | Host 或 Guest | 另一方 | Host 開房時選擇自己的顏色，預設執黑 |
| 本機雙人 | 玩家 | 玩家 | 同一台電腦上兩人輪流點擊棋盤 |

### 2.2 模式
| 模式 | 說明 |
|---|---|
| 正常對戰 | 不計時 |
| 限時 | 每步限時，規則見 §3.2 |

### 2.3 功能需求
| 編號 | 功能 | 說明 |
|---|---|---|
| F1 | 落子 | 點擊棋盤交叉點落子；非法位置無反應 |
| F2 | 勝負判定 | 每步後判斷連五、和局、超時 |
| F3 | 悔棋 | 規則見 §7.2 |
| F4 | 存檔 | 將對局（含時間狀態）存成檔案，可在之後讀回繼續 |
| F5 | 讀檔 | 讀回存檔繼續對局；檔案格式錯誤或棋步不合法時拒絕並提示 |
| F6 | 棋譜紀錄 | 對局結束後可匯出棋譜 |
| F7 | 棋譜回放 | 讀入棋譜，可上一步／下一步／跳到開頭結尾／自動播放 |
| F8 | AI 提示 | 在棋盤上標示建議落點，不自動落子 |
| F9 | 認輸 | 任一玩家可認輸 |

### 2.4 非功能需求
| 編號 | 需求 |
|---|---|
| N1 | AI 思考期間 UI 不可卡頓，可正常拖動視窗與點按鈕 |
| N2 | AI 在困難難度下每步不超過 5 秒（限時模式下另受剩餘時間限制） |
| N3 | core 與 ai 模組有單元測試，CI 每次 push 自動編譯並跑測試 |
| N4 | 相同局面、相同參數、相同時間來源行為下，AI 結果可重現（測試以假時鐘達成）。實際對局受時間上限影響，不同速度的電腦可能搜到不同深度而下出不同的步 |

---

## 3. 遊戲規則

### 3.1 基本規則
- 棋盤 15×15，座標 `(row, col)`，皆從 0 開始，`(0,0)` 在左上角
- 黑先，輪流落子，只能下在空的交叉點
- 連成五子（含）以上即勝，無禁手
- 棋盤下滿且無人連五為和局

### 3.2 限時規則
1. **每步限時**：開局前設定每步秒數，範圍 5–300 秒，預設 30 秒。
2. **計時方式**：輪到某方時開始倒數；該方落子後計時停止，換對方從完整時間開始倒數。
3. **超時**：剩餘時間歸零立即判負，結果原因為 `Timeout`。超時後送達的落子一律無效。
4. **悔棋**：回到被撤銷那步之前的局面，輪到的那方的剩餘時間恢復為**他當初下那步時所剩的時間**（＝每步限時 − 該步已用時間）。
   > 例：限時 30 秒，黑方花 12 秒下了第 5 步。悔棋撤銷第 5 步後，輪到黑方，黑方剩 18 秒。
5. **AI 提示**：提示計算期間計時暫停，提示結果顯示（或取消）後恢復計時。
6. **電腦方**：同樣受每步限時約束，AI 搜尋時間上限 = min(難度設定上限, 剩餘時間 − 300 ms 緩衝)。
7. **連線**：計時以 Host 為準，詳見 §6.2。

---

## 4. 架構

### 4.1 分層

```
┌────────────────────────────────────────────┐
│ ui    MainWindow, BoardView, 對話框, ClockWidget │
├────────────────────────────────────────────┤
│ app   GameController, Player（Human/AI/Remote） │
├───────────────┬───────────────┬────────────┤
│ net           │ ai  AIEngine,  │ format     │
│ NetworkSession│     AIWorker   │ SaveFormat │
├───────────────┴───────────────┴────────────┤
│ core  Board, Rules, MoveHistory, GameState,  │
│       GameClock, GameRecord, ReplayCursor    │
│       （純 C++，無 Qt）                       │
└────────────────────────────────────────────┘
```
依賴方向只能往下：ui / net / app → ai / format → core。core 不 include 任何 Qt 標頭，因此可以單獨用 GoogleTest 測試。
format 只依賴 core 與 Qt6::Core（`QJsonDocument`），不依賴 ai、net、ui、app；ui 與 app 都可以使用它，避免 ui 與 app 互相依賴。

### 4.2 職責對應

| 職責 | 模組 | 主要類別 |
|---|---|---|
| 記錄棋盤每個位置的狀態 | core | `Board` |
| 判斷能不能下、有沒有連五 | core | `Rules` |
| 記錄每一步棋的順序 | core | `MoveHistory` |
| 限時模式計時 | core | `GameClock` |
| 存檔內容、讀檔驗證 | core | `GameRecord` |
| 存讀檔、棋譜的 JSON 格式 | format | `SaveFormat` |
| 回放時逐步前進／後退 | core | `ReplayCursor` |
| 幫電腦決定下哪裡 | ai | `AIEngine`、`Evaluator`、`AIWorker` |
| 和另一台電腦傳送棋步 | net | `NetworkSession` |
| 管理輪到誰、開始與結束 | app | `GameController` |
| 畫棋盤、接收點擊 | ui | `BoardView` |

### 4.3 Player 抽象
`GameController` 不直接區分對戰類型，而是持有黑、白兩個 `Player`：

| 實作 | 取得下一步的方式 |
|---|---|
| `HumanPlayer` | 等待 `BoardView` 的點擊 |
| `AIPlayer` | 交給 `AIWorker` 在背景執行緒搜尋 |
| `RemotePlayer` | 等待 `NetworkSession` 收到對方棋步 |

四種對戰類型只是不同的 Player 組合（本機雙人＝兩個 `HumanPlayer`），GameController 的流程只有一套。

---

## 5. 模組設計

以下為公開介面草案，實作時可調整，但改動需同步更新本文件。

### 5.1 core

```cpp
enum class Stone : std::uint8_t { Empty, Black, White };
Stone opponent(Stone s);

struct Pos {
    int row;
    int col;
    bool operator==(const Pos&) const = default;
};

struct Move {
    Pos pos;
    Stone color;
    std::int64_t timeUsedMs;   // 該步花費的時間；非限時模式為 0
};

enum class GameResult  { Ongoing, BlackWin, WhiteWin, Draw };
enum class ResultReason { None, FiveInRow, Timeout, BoardFull, Resign, Disconnect };
```

**Board**
```cpp
class Board {
public:
    static constexpr int kSize = 15;
    Stone at(Pos p) const;
    bool  inBounds(Pos p) const;
    bool  isEmpty(Pos p) const;
    void  place(Pos p, Stone s);   // 前置條件：inBounds 且 isEmpty
    void  remove(Pos p);
    int   stoneCount() const;
    std::uint64_t hash() const;    // Zobrist hash，供 AI 置換表使用，place/remove 時增量更新
};
```

**Rules**（無狀態）
```cpp
namespace Rules {
    bool isLegal(const Board& b, Pos p);
    bool makesFive(const Board& b, Pos lastMove);   // 檢查 lastMove 所在四個方向是否 ≥ 5 連
    bool isFull(const Board& b);
}
```

**MoveHistory**
```cpp
class MoveHistory {
public:
    void push(const Move& m);
    std::optional<Move> pop();
    const std::vector<Move>& moves() const;
    std::size_t size() const;
};
```

**ITimeSource**（時間來源介面；AI 搜尋與 GameClock 共用）
```cpp
class ITimeSource {
public:
    virtual ~ITimeSource() = default;
    virtual std::int64_t nowMs() const = 0;   // 單調遞增的毫秒數，起點不限
};
```
core 只定義介面，不讀系統時間。測試使用假時鐘（可設定為每次讀取自動前進固定毫秒數）；實際讀時間的實作放在 app 層（里程碑 5，例如以 `QElapsedTimer` 實作）。

**GameClock**（時間來源以介面注入，測試時使用假時鐘）
```cpp
class GameClock {
public:
    GameClock(const ITimeSource& time, std::int64_t moveLimitMs);
    void startTurn();                              // 從完整限時開始倒數
    void startTurnWith(std::int64_t remainingMs);  // 悔棋或讀檔時使用
    std::int64_t stopTurn();                       // 停止並回傳本步已用時間
    void pause();                                  // AI 提示時使用
    void resume();
    std::int64_t remainingMs() const;
    bool isExpired() const;
    bool isRunning() const;
};
```

**GameState**
```cpp
class GameState {
public:
    bool play(Pos p, std::int64_t timeUsedMs);   // 合法則落子、記錄、更新結果
    bool undo(int plies);                         // 回傳是否成功；回傳後可從 lastUndone() 取得被撤銷的步
    void finish(GameResult r, ResultReason why);  // 超時、認輸、斷線時由外部呼叫；已有結果或 r 為 Ongoing 時忽略（先發生者為準）
    const Board&       board() const;
    const MoveHistory& history() const;
    Stone        sideToMove() const;
    GameResult   result() const;
    ResultReason reason() const;
    const std::vector<Move>& lastUndone() const;  // 最近一次成功 undo 撤銷的步（新到舊）；undo 失敗時為空
};
```

**GameRecord**：存檔／棋譜的內容（純資料），以及從空盤重播驗證的邏輯。JSON 轉換不在 core，見 §5.6。
```cpp
enum class MatchType  { Local, HumanVsAi, AiVsAi, Lan };
enum class PlayerType { Human, Ai };
enum class Difficulty { Easy, Normal, Hard };

struct PlayerInfo {
    PlayerType type;
    std::optional<Difficulty> difficulty;   // 只有 Ai 有
};

struct TimeControl {
    bool timed;                  // false = "none"，true = "per_move"
    std::int64_t moveLimitMs;    // 非限時為 0
};

struct GameRecord {
    std::string createdAt;                    // ISO 8601，由呼叫端提供
    MatchType   matchType;
    PlayerInfo  black, white;
    TimeControl timeControl;
    std::vector<Move> moves;                  // color 由黑先輪流推得
    std::optional<std::int64_t> remainingMs;  // 限時且未結束時才有
    GameResult   result;                      // 未結束為 Ongoing
    ResultReason reason;
};

// 從空盤重播並依 §6.1 驗證；成功回傳重建的 GameState，失敗回傳錯誤訊息（含第幾手）
std::variant<GameState, std::string> restore(const GameRecord& r);
```

**ReplayCursor**：回放時在棋步之間移動，不修改棋譜。
```cpp
class ReplayCursor {
public:
    explicit ReplayCursor(std::vector<Move> moves);
    std::size_t index() const;            // 目前顯示前幾手，0 = 空盤
    std::size_t size() const;             // 總手數
    const Board& board() const;
    std::optional<Pos> lastMove() const;
    bool toStart();                       // 以下皆回傳是否有移動；已在邊界時不動
    bool prev();
    bool next();
    bool toEnd();
};
```

### 5.2 ai

```cpp
struct SearchParams {
    int maxDepth;
    std::int64_t timeLimitMs;
    int candidateRadius = 2;              // 只考慮已有棋子周圍 N 格內的空點
    int maxCandidates   = 20;             // 排序後只展開前 N 個候選步
    bool useTranspositionTable = false;
};

struct SearchResult {
    Pos best;
    int score;              // 以 side 的角度；勝負分數見 §5.2.1
    int depthReached;       // 最後一個完整搜完的深度；0 = 由戰術規則直接決定，或深度 1 未完成
    std::int64_t nodes;
    bool cancelled;         // 因取消旗標而停止（時間到不算取消）
};

class AIEngine {
public:
    // 前置條件：對局進行中、棋盤未滿、輪到 side 下
    SearchResult search(const Board& b, Stone side, const SearchParams& params,
                        const std::atomic<bool>& cancel, const ITimeSource& time);
};

namespace Evaluator {
    enum class Pattern { None, Two, OpenTwo, Three, OpenThree, Four, OpenFour, Five };
    int evaluate(const Board& b, Stone side);                          // 局面分，以 side 的角度
    int pointGain(const Board& b, Pos p, Stone side);                  // side 下在 p 時，通過 p 的四條線上 side 棋型分數的增加量
    std::vector<Pattern> patternsInLine(const std::vector<Stone>& line, Stone side);  // 測試用
}

// 候選步：依 §5.2.1 產生並排序，最多 maxCount 個
std::vector<Pos> generateCandidates(const Board& b, Stone side, int radius, int maxCount);

SearchParams paramsFor(Difficulty d);   // Difficulty 定義於 core（§5.1）
SearchParams hintParams();
```

#### 5.2.1 搜尋
- **戰術優先**（搜尋前，`depthReached = 0`）：
  1. 自己有能連五的點 → 直接下（多個時取排序第一個）
  2. 對方有能連五的點 → 下在該點擋住（多個時擋不完，取排序第一個）
  3. 否則進入一般搜尋
- **迭代加深**：深度 1, 2, 3… 到 `maxDepth` 逐層搜尋；時間到或被取消時，回傳最後一個完整搜完的深度的結果。若深度 1 尚未完成就停止，回傳排序第一的候選步（`depthReached = 0`）。找到必勝或必敗（分數絕對值 ≥ 勝利分數 − 1000）時不再加深
- **Negamax + Alpha-Beta 剪枝**：分數一律以輪到的一方的角度
- **終局分數**：某步造成連五 → 勝利分數 100,000,000 − 層數（越快贏分數越高、越慢輸分數越高）；盤滿 → 0；到達深度 → 評估函數
- **候選步產生**：已有棋子周圍 `candidateRadius` 格內（切比雪夫距離，即 8 方向）的空點；空盤時只有天元 `(7,7)`
- **著法排序**：每個候選點的分數 = 自己下在該點時，通過該點的四條線上己方棋型分數的增加量，加上對方下在該點時對方棋型分數的增加量（進攻 + 防守）。依分數高到低排序，同分時 row 小的優先，再比 col 小的優先；只保留前 `maxCandidates` 個。根節點上一層的最佳步移到最前面
- **時間與取消檢查**：每搜 256 個節點檢查一次取消旗標與時間；超過 `timeLimitMs` 或被取消時，放棄目前這一層
- **置換表**（`useTranspositionTable` 時）：以 `Board::hash()` 為 key（五子棋中輪到誰由棋子數決定，所以 hash 已足以區分）；固定 2^20 個項目，新結果直接覆蓋；每次 `search()` 開始時清空，確保每次呼叫結果只取決於輸入。存入勝負分數時換算為相對該節點的層數
- 不使用亂數，同分時依固定順序選擇，確保結果可重現

#### 5.2.2 評估函數
對雙方在四個方向上的棋型計分，局面分 = 己方分數 − 對方分數 × 11 / 10（整數運算，略偏重防守）。

**線、區段、棋組**
- **線**：棋盤上某個方向（橫、直、兩條斜線）的一整列格子；長度不到 5 的斜線忽略。
- **區段**：評估某一方時，對方的棋子與棋盤邊界視為阻隔，把線切成數段。長度不到 5 的區段無法成五，不計分。
- **棋組**：區段內己方棋子依「相鄰兩子之間最多隔 1 個空點」分組。例如 `XX_X` 是一組，`XX__X` 是兩組。
- 每個棋組**單獨**判定棋型（同區段的其他棋組視為空點），只歸類為一種（取最強的），分數相加。

**棋型判定**（「成五」= 連成 5 子以上；「放一子」只考慮同區段內的空點）

| 棋型 | 判定 | 例（X 己方，O 對方或邊界，_ 空） | 分數（初值，待調整） |
|---|---|---|---|
| 連五 | 已有 5 子以上相連 | `XXXXX` | 10,000,000 |
| 活四 | 至少 2 個空點各自能成五 | `_XXXX_`、`X_XXX_X` | 100,000 |
| 衝四 | 恰好 1 個空點能成五 | `OXXXX_`、`X_XXX`、`XX_XX` | 10,000 |
| 活三 | 不是四，且放一子後能成為活四 | `__XXX_`、`_XX_X_` | 5,000 |
| 眠三 | 不是活三，但放一子後能成為衝四 | `OXXX__`、`X_X_X`、`O_XXX_O` | 500 |
| 活二 | 不是三，且放一子後能成為活三 | `__XX__`、`_X_X__` | 200 |
| 眠二 | 不是活二，但放一子後能成為眠三 | `OXX___`、`OX_X__` | 50 |
| 無 | 以上皆非（含單子） | `X`、`OXXO` | 0 |

判定只取決於「區段長度」與「棋組在區段內的位置」，實作可預先計算或快取。

#### 5.2.3 難度
| 難度 | maxDepth | timeLimitMs | 其他 |
|---|---|---|---|
| 簡單 | 2 | 1,000 | maxCandidates = 8 |
| 普通 | 4 | 3,000 | — |
| 困難 | 10（迭代加深） | 5,000 | 啟用置換表 |
| 提示 | 10（同困難） | 3,000 | 啟用置換表（同困難） |

未列出的參數使用 `SearchParams` 預設值。限時模式下 `timeLimitMs` 再取 min(上表數值, 剩餘時間 − 300 ms)。

**難度的測試標準**（基本戰術，見 §8）：簡單需通過 a–c；普通需通過 a–e；困難需通過 a–g。

#### 5.2.4 執行緒
- `AIWorker`（QObject）以 `moveToThread` 放進專用 `QThread`
- `GameController` 透過 signal 發出搜尋請求，附上 `requestId`
- `AIWorker` 完成後以 signal 回傳 `(requestId, SearchResult)`
- 悔棋、讀檔、離開對局、超時時：設定取消旗標，並遞增 `requestId`；收到舊 `requestId` 的結果直接丟棄

### 5.3 net

#### 5.3.1 角色
- Host 以 `QTcpServer` 監聽，預設 port **45678**（可在開房對話框修改）
- Guest 輸入 Host 的 IP 與 port，以 `QTcpSocket` 連線
- Host 持有唯一權威的 `GameState` 與 `GameClock`；Host 自己的落子也走同一套驗證流程

#### 5.3.2 協定
每則訊息為一行 UTF-8 JSON，以 `\n` 結尾。每則訊息都有 `type` 欄位。

| type | 方向 | 欄位 | 說明 |
|---|---|---|---|
| `HELLO` | G→H | `version`, `name` | 連線後第一則訊息 |
| `WELCOME` | H→G | `yourColor`, `timed`, `moveLimitMs` | 開局資訊；版本不符則改送 `REJECT` 並斷線 |
| `MOVE_REQUEST` | G→H | `row`, `col` | Guest 想下的位置 |
| `MOVE` | H→G | `row`, `col`, `color`, `moveNo` | Host 確認後的棋步（雙方的棋步都會送） |
| `REJECT` | H→G | `reason` | 落子被拒（不合法、不是你的回合、已超時） |
| `CLOCK` | H→G | `side`, `remainingMs` | 限時模式下每 500 ms 廣播一次 |
| `UNDO_REQUEST` | 雙向 | — | 請求悔棋 |
| `UNDO_REPLY` | 雙向 | `accept` | 回覆悔棋請求 |
| `UNDO` | H→G | `plies`, `sideToMove`, `remainingMs` | Host 執行悔棋後的結果 |
| `RESIGN` | 雙向 | — | 認輸 |
| `GAME_OVER` | H→G | `result`, `reason` | 對局結束 |
| `PING` / `PONG` | 雙向 | — | 每 2 秒一次心跳；10 秒未收到任何訊息視為斷線 |

斷線時，仍在線的一方判勝，原因 `Disconnect`。

### 5.4 ui
| 元件 | 說明 |
|---|---|
| `MainWindow` | 選單（新對局、存檔、讀檔、匯出棋譜、回放）、工具列（悔棋、提示、認輸） |
| `BoardView` | QPainter 繪製格線、星位、棋子；標示最後一步與提示位置；點擊時將座標換算為最近的交叉點，距離超過格距 40% 則忽略 |
| `NewGameDialog` | 選擇對戰類型、顏色、難度、模式、每步秒數 |
| `NetworkDialog` | 開房（顯示本機 IP 與 port）或加入（輸入 IP 與 port） |
| `ClockWidget` | 顯示雙方剩餘時間；剩 5 秒以下變紅 |
| `ReplayControls` | 回放時的開頭／上一步／播放・暫停／下一步／結尾，以及結束回放；自動播放固定每步 1 秒，到最後一手自動停止。回放中棋盤不可落子，狀態列顯示「回放：第 n / N 手」 |

### 5.5 app — GameController 狀態

```
Idle ──開局──▶ WaitingMove ──收到合法步──▶ WaitingMove（換邊）
                  │  │                         │
                  │  └─提示──▶ WaitingHint ─結果─┘（回到 WaitingMove）
                  │
                  └─連五／和局／超時／認輸／斷線──▶ GameOver
Idle ──讀棋譜──▶ Replaying
```
- 限時模式下，`GameController` 用 QTimer 每 100 ms 檢查 `GameClock::isExpired()`
- 落子到達時再檢查一次：若該步用時超過限時，視為超時，落子無效

### 5.6 format

```cpp
namespace SaveFormat {
    QByteArray toJson(const core::GameRecord& r);
    // 解析並檢查欄位；成功回傳 GameRecord，失敗回傳錯誤訊息。棋步合法性再交給 core::restore 驗證
    std::variant<core::GameRecord, QString> fromJson(const QByteArray& bytes);
}
```
- `createdAt` 由呼叫端填入 `GameRecord`，SaveFormat 不讀系統時間，測試可重現

---

## 6. 資料格式

### 6.1 存檔與棋譜
存檔與棋譜共用同一格式，副檔名 `.gomoku.json`。對局未結束時 `result` 為 `null`。

```json
{
  "format": "gomoku",
  "version": 1,
  "createdAt": "2026-10-04T21:40:00+08:00",
  "matchType": "human_vs_ai",
  "players": {
    "black": { "type": "human" },
    "white": { "type": "ai", "difficulty": "hard" }
  },
  "timeControl": { "mode": "per_move", "moveLimitMs": 30000 },
  "moves": [
    { "r": 7, "c": 7, "t": 4210 },
    { "r": 7, "c": 8, "t": 1830 }
  ],
  "current": { "remainingMs": 18000 },
  "result": null
}
```

| 欄位 | 說明 |
|---|---|
| 欄位 | 必要 | 說明 |
|---|---|---|
| `format` | 是 | 固定為 `"gomoku"` |
| `version` | 是 | 目前為 `1`；其他值拒絕 |
| `createdAt` | 是 | ISO 8601 字串，含時區 |
| `matchType` | 是 | `local`、`human_vs_ai`、`ai_vs_ai`、`lan` |
| `players.black` / `players.white` | 是 | `{ "type": "human" }` 或 `{ "type": "ai", "difficulty": "easy" \| "normal" \| "hard" }`。須與 `matchType` 一致：`local`、`lan` 雙方皆為 `human`；`human_vs_ai` 恰一方為 `ai`；`ai_vs_ai` 雙方皆為 `ai` |
| `timeControl.mode` | 是 | `none` 或 `per_move` |
| `timeControl.moveLimitMs` | `per_move` 時 | 每步限時毫秒數，範圍 5000–300000（§3.2） |
| `moves[]` | 是 | 可為空陣列；每項 `r`、`c` 為整數，`t` 為 ≥ 0 的整數 |
| `moves[].t` | 是 | 該步已用毫秒數；非限時模式為 0。悔棋恢復時間依此計算 |
| `current.remainingMs` | `per_move` 且未結束時 | 存檔當下輪到的一方剩餘時間；其他情況省略 |
| `result` | 是 | 未結束為 `null`；結束為 `{ "winner": ..., "reason": ... }`，見下表 |

`result` 的值：

| `reason` | `winner` | 對應 `ResultReason` |
|---|---|---|
| `five_in_row` | `black` / `white` | `FiveInRow` |
| `board_full` | `null` | `BoardFull` |
| `timeout` | `black` / `white` | `Timeout` |
| `resign` | `black` / `white` | `Resign` |
| `disconnect` | `black` / `white` | `Disconnect` |

**讀檔驗證**：先檢查必要欄位與型別，缺欄位、型別錯誤、`format` 或 `version` 不符即拒絕；不認得的欄位忽略（保留向後相容）。接著從空盤依序重播 `moves`，每一步都用 `Rules::isLegal` 檢查，任何一步不合法即拒絕。最後比對結果：

| `result` | 重播完畢後必須是 |
|---|---|
| `null` | 對局仍在進行（最後一步沒有造成連五或盤滿） |
| `five_in_row` | 最後一步造成連五，且勝方與 `winner` 相同 |
| `board_full` | 盤面已滿且無人連五 |
| `timeout`、`resign`、`disconnect` | 對局仍在進行（無法從棋步驗證），再以 `GameState::finish()` 套用結果 |

另外，任何一步之後若已分出勝負但後面還有棋步，同樣拒絕。

**讀檔繼續與回放的支援範圍**：回放不需要玩家與計時資訊，任何 `matchType` 都可回放。讀檔後繼續對局則只支援程式目前已實作的類型（里程碑 3 時只有 `local` 且 `none`），其他類型提示「尚未支援」並拒絕。

### 6.2 連線計時
- 只有 Host 的 `GameClock` 會判定超時
- Guest 依 `CLOCK` 訊息顯示時間，兩次訊息之間自行倒數以保持畫面流暢，但不做判定
- 是否超時以 Host 收到 `MOVE_REQUEST` 的時間為準

---

## 7. 主要流程

### 7.1 一步棋
1. `GameController` 通知目前的 `Player` 輪到他，限時模式則 `GameClock::startTurn()`
2. Player 回報落點
3. `GameClock::stopTurn()` 取得用時；若已超時 → GameOver(`Timeout`)
4. `GameState::play()`；不合法則忽略並繼續等待
5. 檢查結果：連五 → GameOver(`FiveInRow`)；盤滿 → GameOver(`BoardFull`)
6. 換邊，回到第 1 步

### 7.2 悔棋
| 對戰類型 | 行為 |
|---|---|
| 人對電腦 | 撤銷兩步（AI 的一步 + 玩家的一步），回到玩家的回合。若 AI 正在思考，先取消搜尋，再撤銷玩家的一步 |
| 電腦對電腦 | 不提供悔棋 |
| 本機雙人 | 撤銷一步，回到下那一步的一方的回合 |
| 區網連線 | 需對方同意。等待回覆期間計時暫停；15 秒未回覆視為拒絕。同意後撤銷到請求方的回合 |

共同規則：
- 對局結束後不可悔棋
- 沒有可撤銷的步時按鈕停用
- 限時模式下，悔棋後輪到的那方剩餘時間 = 每步限時 − 被撤銷那步的 `timeUsedMs`（見 §3.2 第 4 點）

### 7.3 AI 提示
1. 只有輪到人類玩家時可用；區網連線模式不提供（見 §11）
2. 限時模式下 `GameClock::pause()`
3. 以「提示」參數搜尋
4. 在棋盤上標示建議落點，`GameClock::resume()`
5. 玩家落子後標示消失

---

## 8. 測試計畫

| 模組 | 測試情境 |
|---|---|
| Board | 落子、移除、邊界檢查；Zobrist hash 在 place/remove 後與重新計算一致 |
| Rules | 橫、直、兩斜四方向連五；六連也算勝；四子不算勝；棋盤邊緣與角落；非法位置（越界、已有棋子） |
| MoveHistory | push/pop 順序；空時 pop 回傳空值 |
| GameClock | 倒數正確；歸零 `isExpired`；`stopTurn` 回傳用時；`pause` 期間不扣時間；`startTurnWith` 恢復指定時間（全部用假時鐘，不 sleep） |
| GameState | 黑先；輪流；勝負後拒絕落子；悔棋後局面與輪次正確；盤滿和局 |
| 限時規則 | 超時判負；超時後的落子無效；悔棋恢復 18 秒的範例；提示期間不扣時間 |
| GameRecord | 合法棋譜重播後局面一致；非法棋步（越界、重複）拒絕；分出勝負後仍有棋步拒絕；`result` 與重播不符拒絕；timeout／resign 套用成功 |
| SaveFormat | 存檔後讀回完全一致；不合法 JSON 不崩潰；缺欄位、型別錯誤、`format`／版本不符時拒絕；未知欄位忽略；§6.1 範例可讀入 |
| ReplayCursor | 前進／後退／開頭／結尾的局面正確；在邊界時不移動；空棋譜 |
| Evaluator | 各棋型辨識正確（每種棋型至少一個正例、一個反例，含棋盤邊緣、中間有空格、被對方擋住）；局面分以 side 角度且對稱 |
| 候選步 | 空盤只回傳天元；只含半徑內空點；排序與同分順序固定；數量上限 |
| AIEngine | 基本戰術（每項至少一個局面，另加鏡像或旋轉版本）：a. 自己一步連五必下；b. 對方有四（衝四或活四）必擋；c. 雙方都有四時先連五；d. 對方活三必擋；e. 自己能做活四且對方無四時做活四；f. 能下四三時下四三；g. 對方下一步能成四三或雙三時先防守。另測：相同輸入結果一致；時間上限內回傳；取消後立即返回；深度 1 未完成即取消仍回傳合法步；置換表開關在固定深度下結果相同（全部使用假時鐘） |
| 協定（net） | 訊息序列化／反序列化；不合法 JSON 不會崩潰 |

連線、UI 以手動測試為主，測試步驟記錄於 `docs/manual-test.md`。

---

## 9. 里程碑

| # | 內容 | 完成標準 |
|---|---|---|
| 0 | 專案骨架 | CMake 能編出空視窗；一個空測試通過；CI 綠燈 |
| 1 | core：Board、Rules、MoveHistory、GameState | 對應測試全過 |
| 2 | UI：畫棋盤、點擊落子 | 能在單機上完成一局並判定勝負 |
| 3 | 存讀檔、棋譜、回放 | SaveFormat 測試全過；回放可操作 |
| 4 | AI：搜尋、評估、難度 | AIEngine 測試全過；困難難度能擋住基本戰術 |
| 5 | AI 背景執行緒、人對電腦、電腦對電腦、AI 提示 | AI 思考時 UI 不卡；悔棋能取消搜尋 |
| 6 | 限時模式 | GameClock 與限時規則測試全過 |
| 7 | 區網連線 | 兩台電腦能完成一局，含悔棋、超時、斷線 |
| 8 | 收尾 | 手動測試清單全部通過 |

---

## 10. 設計權衡

| 問題 | 選擇 | 理由 |
|---|---|---|
| 連線架構：client-server 或 P2P | client-server（Host 兼任 server） | 只有一方持有權威狀態，避免兩邊不同步；兩人對戰不需要另外架設伺服器 |
| GUI 框架：Qt 或 SFML | Qt | 視窗、對話框、網路、多執行緒都有完整支援 |
| AI 演算法：Minimax + Alpha-Beta 或 MCTS | Minimax + Alpha-Beta | 除錯容易、戰術計算強；五子棋分支多但可用候選步限制 |
| 計時放在哪一層 | core，時間來源注入 | 可用假時鐘測試超時，不需真的等待 |
| 對戰類型怎麼實作 | Player 抽象 | 四種對戰類型共用同一套 GameController 流程 |
| 連線協定格式 | 一行一則 JSON | 好讀好除錯，Qt 內建 JSON 支援；傳輸量很小，不需二進位格式 |
| 悔棋時間 | 恢復當時剩餘時間，而非重新給滿 | 避免利用悔棋獲得額外思考時間 |

---

## 11. 待確認

以下目前採用預設值，可以改：

1. **本機雙人對戰**：已決定加入（v0.2），以兩個 `HumanPlayer` 實作；悔棋每次撤銷一步（見 §7.2）。
2. **區網連線的 AI 提示**：目前不提供（公平性，且暫停計時會讓對方空等）。
3. **區網對局能否存檔續玩**：目前只能在結束後匯出棋譜，不能存到一半之後再連線續玩。
4. **SaveFormat 的 JSON 處理**：已決定（v0.3）。core 提供 `GameRecord` 與重播驗證；JSON 轉換放在獨立的 `format` 模組，使用 `QJsonDocument`（見 §4.1、§5.6）。不放在 app，是因為 ui 也需要存讀檔，放在 app 會造成 ui 與 app 互相依賴。
5. **電腦對電腦**：是否需要暫停／繼續、調整播放速度？
6. **覆蓋目前對局的確認**：新對局、讀檔、回放會直接取代目前對局，目前不詢問。之後再決定是否加入「尚未儲存」的確認。