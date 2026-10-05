#include <gtest/gtest.h>

#include <variant>

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>

#include "GameRecord.h"
#include "SaveFormat.h"

using namespace core;

namespace {

// SDD §6.1 的範例，原封不動
const char* kSddExample = R"({
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
})";

QJsonObject example() {
    return QJsonDocument::fromJson(kSddExample).object();
}

QByteArray bytes(const QJsonObject& o) {
    return QJsonDocument(o).toJson();
}

// 修改巢狀物件中的一個欄位（Qt 的 QJsonObject 是值語意，需要逐層取出再放回）
QJsonObject withChild(QJsonObject root, const QString& child, const QString& key, const QJsonValue& v) {
    QJsonObject c = root[child].toObject();
    c[key] = v;
    root[child] = c;
    return root;
}

QJsonObject withoutChild(QJsonObject root, const QString& child, const QString& key) {
    QJsonObject c = root[child].toObject();
    c.remove(key);
    root[child] = c;
    return root;
}

QJsonObject withFirstMove(QJsonObject root, const QString& key, const QJsonValue& v) {
    QJsonArray moves = root["moves"].toArray();
    QJsonObject m = moves[0].toObject();
    if (v.isUndefined()) {
        m.remove(key);
    } else {
        m[key] = v;
    }
    moves[0] = m;
    root["moves"] = moves;
    return root;
}

bool rejected(const QJsonObject& o) {
    return std::holds_alternative<QString>(SaveFormat::fromJson(bytes(o)));
}

GameRecord load(const QByteArray& b) {
    auto out = SaveFormat::fromJson(b);
    if (const auto* err = std::get_if<QString>(&out)) {
        ADD_FAILURE() << "fromJson failed: " << err->toStdString();
        return {};
    }
    return std::get<GameRecord>(out);
}

GameRecord sampleRecord() {
    GameRecord r;
    r.createdAt = "2026-10-05T10:00:00+08:00";
    r.matchType = MatchType::Local;
    r.moves = {{{7, 7}, Stone::Black, 0}, {{7, 8}, Stone::White, 0}, {{8, 8}, Stone::Black, 0}};
    return r;
}

void expectSame(const GameRecord& a, const GameRecord& b) {
    EXPECT_EQ(a.createdAt, b.createdAt);
    EXPECT_EQ(a.matchType, b.matchType);
    EXPECT_EQ(a.black, b.black);
    EXPECT_EQ(a.white, b.white);
    EXPECT_EQ(a.timeControl, b.timeControl);
    ASSERT_EQ(a.moves.size(), b.moves.size());
    for (std::size_t i = 0; i < a.moves.size(); ++i) {
        EXPECT_EQ(a.moves[i].pos, b.moves[i].pos) << "move " << i;
        EXPECT_EQ(a.moves[i].color, b.moves[i].color) << "move " << i;
        EXPECT_EQ(a.moves[i].timeUsedMs, b.moves[i].timeUsedMs) << "move " << i;
    }
    EXPECT_EQ(a.remainingMs, b.remainingMs);
    EXPECT_EQ(a.result, b.result);
    EXPECT_EQ(a.reason, b.reason);
}

} // namespace

// ---- 寫出後讀回 ----

TEST(SaveFormat, RoundTripLocalOngoing) {
    const GameRecord r = sampleRecord();
    expectSame(load(SaveFormat::toJson(r)), r);
}

TEST(SaveFormat, RoundTripEmptyGame) {
    GameRecord r = sampleRecord();
    r.moves.clear();
    expectSame(load(SaveFormat::toJson(r)), r);
}

TEST(SaveFormat, RoundTripTimedAiOngoingKeepsRemainingTime) {
    GameRecord r = sampleRecord();
    r.matchType = MatchType::HumanVsAi;
    r.white = {PlayerType::Ai, Difficulty::Hard};
    r.timeControl = {true, 30000};
    r.moves[0].timeUsedMs = 4210;
    r.moves[1].timeUsedMs = 1830;
    r.moves[2].timeUsedMs = 12000;
    r.remainingMs = 18000;
    expectSame(load(SaveFormat::toJson(r)), r);
}

TEST(SaveFormat, RoundTripEveryMatchTypeAndDifficulty) {
    for (const MatchType t : {MatchType::Local, MatchType::HumanVsAi, MatchType::AiVsAi, MatchType::Lan}) {
        for (const Difficulty d : {Difficulty::Easy, Difficulty::Normal, Difficulty::Hard}) {
            GameRecord r = sampleRecord();
            r.matchType = t;
            r.black = {PlayerType::Ai, d};
            expectSame(load(SaveFormat::toJson(r)), r);
        }
    }
}

TEST(SaveFormat, RoundTripEveryResult) {
    const std::pair<GameResult, ResultReason> results[] = {
        {GameResult::BlackWin, ResultReason::FiveInRow}, {GameResult::WhiteWin, ResultReason::FiveInRow},
        {GameResult::Draw, ResultReason::BoardFull},     {GameResult::WhiteWin, ResultReason::Timeout},
        {GameResult::BlackWin, ResultReason::Resign},    {GameResult::WhiteWin, ResultReason::Disconnect},
    };
    for (const auto& [result, reason] : results) {
        GameRecord r = sampleRecord();
        r.result = result;
        r.reason = reason;
        expectSame(load(SaveFormat::toJson(r)), r);
    }
}

// ---- 寫出的內容 ----

TEST(SaveFormat, ToJsonWritesSddFieldNames) {
    GameRecord r = sampleRecord();
    r.result = GameResult::BlackWin;
    r.reason = ResultReason::Resign;
    const QJsonObject o = QJsonDocument::fromJson(SaveFormat::toJson(r)).object();
    EXPECT_EQ(o["format"].toString(), "gomoku");
    EXPECT_EQ(o["version"].toInt(), 1);
    EXPECT_EQ(o["matchType"].toString(), "local");
    EXPECT_EQ(o["players"]["black"]["type"].toString(), "human");
    EXPECT_EQ(o["timeControl"]["mode"].toString(), "none");
    EXPECT_EQ(o["moves"][1]["r"].toInt(), 7);
    EXPECT_EQ(o["moves"][1]["c"].toInt(), 8);
    EXPECT_EQ(o["moves"][1]["t"].toInt(), 0);
    EXPECT_EQ(o["result"]["winner"].toString(), "black");
    EXPECT_EQ(o["result"]["reason"].toString(), "resign");
}

TEST(SaveFormat, ToJsonOmitsFieldsThatDoNotApply) {
    const QJsonObject o = QJsonDocument::fromJson(SaveFormat::toJson(sampleRecord())).object();
    EXPECT_FALSE(o.contains("current"));                               // 不限時
    EXPECT_FALSE(o["timeControl"].toObject().contains("moveLimitMs"));  // 不限時
    EXPECT_FALSE(o["players"]["black"].toObject().contains("difficulty"));  // human
    EXPECT_TRUE(o.contains("result"));
    EXPECT_TRUE(o["result"].isNull());                                  // 未結束
}

TEST(SaveFormat, ToJsonWritesDrawWinnerAsNull) {
    GameRecord r = sampleRecord();
    r.result = GameResult::Draw;
    r.reason = ResultReason::BoardFull;
    const QJsonObject o = QJsonDocument::fromJson(SaveFormat::toJson(r)).object();
    EXPECT_TRUE(o["result"]["winner"].isNull());
    EXPECT_EQ(o["result"]["reason"].toString(), "board_full");
}

// ---- 讀入 ----

TEST(SaveFormat, SddExampleLoads) {
    const GameRecord r = load(kSddExample);
    EXPECT_EQ(r.createdAt, "2026-10-04T21:40:00+08:00");
    EXPECT_EQ(r.matchType, MatchType::HumanVsAi);
    EXPECT_EQ(r.black, (PlayerInfo{PlayerType::Human, std::nullopt}));
    EXPECT_EQ(r.white, (PlayerInfo{PlayerType::Ai, Difficulty::Hard}));
    EXPECT_EQ(r.timeControl, (TimeControl{true, 30000}));
    ASSERT_EQ(r.moves.size(), 2u);
    EXPECT_EQ(r.moves[0].pos, (Pos{7, 7}));
    EXPECT_EQ(r.moves[0].color, Stone::Black);
    EXPECT_EQ(r.moves[0].timeUsedMs, 4210);
    EXPECT_EQ(r.moves[1].pos, (Pos{7, 8}));
    EXPECT_EQ(r.moves[1].color, Stone::White);
    EXPECT_EQ(r.remainingMs, 18000);
    EXPECT_EQ(r.result, GameResult::Ongoing);
    EXPECT_EQ(r.reason, ResultReason::None);
}

TEST(SaveFormat, SddExamplePassesRestore) {
    const auto out = restore(load(kSddExample));
    EXPECT_TRUE(std::holds_alternative<GameState>(out));
}

TEST(SaveFormat, CurrentIgnoredWhenNotRequired) {
    // 不限時時 current 應省略；若出現則忽略
    QJsonObject o = example();
    o["timeControl"] = QJsonObject{{"mode", "none"}};
    const GameRecord r = load(bytes(o));
    EXPECT_EQ(r.remainingMs, std::nullopt);
}

TEST(SaveFormat, UnknownFieldsIgnored) {
    QJsonObject o = example();
    o["comment"] = "hello";
    o = withChild(o, "players", "referee", QJsonObject{{"type", "human"}});
    o = withFirstMove(o, "note", "good move");
    const GameRecord r = load(bytes(o));
    EXPECT_EQ(r.moves.size(), 2u);
}

TEST(SaveFormat, InvalidJsonRejected) {
    for (const QByteArray b : {QByteArray(""), QByteArray("{"), QByteArray("not json"),
                               QByteArray("[]"), QByteArray("null"), QByteArray("\xff\xfe\x00")}) {
        EXPECT_TRUE(std::holds_alternative<QString>(SaveFormat::fromJson(b))) << b.toStdString();
    }
}

TEST(SaveFormat, FormatOrVersionMismatchRejected) {
    QJsonObject o = example();
    o["format"] = "chess";
    EXPECT_TRUE(rejected(o));
    o = example();
    o["version"] = 2;
    EXPECT_TRUE(rejected(o));
    o["version"] = "1";
    EXPECT_TRUE(rejected(o));
}

TEST(SaveFormat, MissingTopLevelFieldsRejected) {
    for (const char* key : {"format", "version", "createdAt", "matchType", "players", "timeControl", "moves",
                            "result"}) {
        QJsonObject o = example();
        o.remove(key);
        EXPECT_TRUE(rejected(o)) << key;
    }
}

TEST(SaveFormat, MissingNestedFieldsRejected) {
    EXPECT_TRUE(rejected(withoutChild(example(), "players", "black")));
    EXPECT_TRUE(rejected(withoutChild(example(), "players", "white")));
    EXPECT_TRUE(rejected(withoutChild(example(), "timeControl", "mode")));
    EXPECT_TRUE(rejected(withoutChild(example(), "timeControl", "moveLimitMs")));   // per_move
    EXPECT_TRUE(rejected(withFirstMove(example(), "r", QJsonValue::Undefined)));
    EXPECT_TRUE(rejected(withFirstMove(example(), "c", QJsonValue::Undefined)));
    EXPECT_TRUE(rejected(withFirstMove(example(), "t", QJsonValue::Undefined)));
}

TEST(SaveFormat, TimedOngoingGameRequiresRemainingTime) {
    QJsonObject o = example();
    o.remove("current");
    EXPECT_TRUE(rejected(o));
    EXPECT_TRUE(rejected(withoutChild(example(), "current", "remainingMs")));
}

TEST(SaveFormat, TimedFinishedGameDoesNotRequireRemainingTime) {
    QJsonObject o = example();
    o.remove("current");
    o["result"] = QJsonObject{{"winner", "black"}, {"reason", "timeout"}};
    const GameRecord r = load(bytes(o));
    EXPECT_EQ(r.remainingMs, std::nullopt);
    EXPECT_EQ(r.result, GameResult::BlackWin);
    EXPECT_EQ(r.reason, ResultReason::Timeout);
}

TEST(SaveFormat, WrongTypesRejected) {
    QJsonObject o = example();
    o["createdAt"] = 123;
    EXPECT_TRUE(rejected(o));
    o = example();
    o["moves"] = QJsonObject{};
    EXPECT_TRUE(rejected(o));
    o = example();
    o["players"] = "human";
    EXPECT_TRUE(rejected(o));
    o = example();
    o["result"] = "black";
    EXPECT_TRUE(rejected(o));
    EXPECT_TRUE(rejected(withFirstMove(example(), "r", "7")));
    EXPECT_TRUE(rejected(withFirstMove(example(), "r", 7.5)));
    EXPECT_TRUE(rejected(withFirstMove(example(), "t", -1)));
    EXPECT_TRUE(rejected(withChild(example(), "timeControl", "moveLimitMs", 0)));
    EXPECT_TRUE(rejected(withChild(example(), "current", "remainingMs", -5)));
}

TEST(SaveFormat, UnknownEnumValuesRejected) {
    QJsonObject o = example();
    o["matchType"] = "online";
    EXPECT_TRUE(rejected(o));
    EXPECT_TRUE(rejected(withChild(example(), "timeControl", "mode", "blitz")));
    EXPECT_TRUE(rejected(withChild(example(), "players", "black", QJsonObject{{"type", "robot"}})));
    EXPECT_TRUE(rejected(withChild(example(), "players", "white",
                                   QJsonObject{{"type", "ai"}, {"difficulty", "insane"}})));
    o = example();
    o["result"] = QJsonObject{{"winner", "red"}, {"reason", "five_in_row"}};
    EXPECT_TRUE(rejected(o));
    o["result"] = QJsonObject{{"winner", "black"}, {"reason", "magic"}};
    EXPECT_TRUE(rejected(o));
}

TEST(SaveFormat, AiPlayerRequiresDifficulty) {
    EXPECT_TRUE(rejected(withChild(example(), "players", "white", QJsonObject{{"type", "ai"}})));
}

TEST(SaveFormat, InvalidCreatedAtRejected) {
    QJsonObject o = example();
    o["createdAt"] = "yesterday";
    EXPECT_TRUE(rejected(o));
}

TEST(SaveFormat, ErrorMessageNamesTheField) {
    const auto out = SaveFormat::fromJson(bytes(withFirstMove(example(), "r", "7")));
    ASSERT_TRUE(std::holds_alternative<QString>(out));
    EXPECT_TRUE(std::get<QString>(out).contains("moves[0].r")) << std::get<QString>(out).toStdString();
}
