#include "SaveFormat.h"

#include <array>
#include <climits>
#include <cmath>
#include <utility>

#include <QDateTime>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>

namespace SaveFormat {

namespace {

using core::Difficulty;
using core::GameResult;
using core::MatchType;
using core::PlayerType;
using core::ResultReason;

constexpr int kVersion = 1;
constexpr std::int64_t kMinMoveLimitMs = 5000;     // SDD §3.2
constexpr std::int64_t kMaxMoveLimitMs = 300000;
const char* const kFormatName = "gomoku";

template <typename E, std::size_t N>
using NameTable = std::array<std::pair<const char*, E>, N>;

constexpr NameTable<MatchType, 4> kMatchTypes{{
    {"local", MatchType::Local},
    {"human_vs_ai", MatchType::HumanVsAi},
    {"ai_vs_ai", MatchType::AiVsAi},
    {"lan", MatchType::Lan},
}};
constexpr NameTable<PlayerType, 2> kPlayerTypes{{
    {"human", PlayerType::Human},
    {"ai", PlayerType::Ai},
}};
constexpr NameTable<Difficulty, 3> kDifficulties{{
    {"easy", Difficulty::Easy},
    {"normal", Difficulty::Normal},
    {"hard", Difficulty::Hard},
}};
constexpr NameTable<ResultReason, 5> kReasons{{
    {"five_in_row", ResultReason::FiveInRow},
    {"board_full", ResultReason::BoardFull},
    {"timeout", ResultReason::Timeout},
    {"resign", ResultReason::Resign},
    {"disconnect", ResultReason::Disconnect},
}};

template <typename E, std::size_t N>
QString nameOf(E value, const NameTable<E, N>& table) {
    for (const auto& [name, e] : table) {
        if (e == value) {
            return QString::fromLatin1(name);
        }
    }
    return {};
}

// ---- 寫出 ----

QJsonObject playerToJson(const core::PlayerInfo& p) {
    QJsonObject o{{"type", nameOf(p.type, kPlayerTypes)}};
    if (p.type == PlayerType::Ai && p.difficulty) {
        o["difficulty"] = nameOf(*p.difficulty, kDifficulties);
    }
    return o;
}

QJsonValue resultToJson(GameResult r, ResultReason why) {
    if (r == GameResult::Ongoing) {
        return QJsonValue::Null;
    }
    QJsonValue winner = QJsonValue::Null;
    if (r == GameResult::BlackWin) {
        winner = "black";
    } else if (r == GameResult::WhiteWin) {
        winner = "white";
    }
    return QJsonObject{{"winner", winner}, {"reason", nameOf(why, kReasons)}};
}

// ---- 讀入 ----
// 讀入時任何錯誤都以 FormatError 丟出，由 fromJson 統一轉成錯誤訊息。

struct FormatError {
    QString message;
};

[[noreturn]] void fail(const QString& message) {
    throw FormatError{message};
}

[[noreturn]] void failType(const QString& path, const char* expected) {
    fail(QStringLiteral("欄位 %1 的型別錯誤，應為%2").arg(path, QString::fromUtf8(expected)));
}

QJsonValue require(const QJsonObject& o, const QString& key, const QString& path) {
    if (!o.contains(key)) {
        fail(QStringLiteral("缺少欄位 %1").arg(path));
    }
    return o.value(key);
}

QJsonObject asObject(const QJsonValue& v, const QString& path) {
    if (!v.isObject()) {
        failType(path, "物件");
    }
    return v.toObject();
}

QJsonArray asArray(const QJsonValue& v, const QString& path) {
    if (!v.isArray()) {
        failType(path, "陣列");
    }
    return v.toArray();
}

QString asString(const QJsonValue& v, const QString& path) {
    if (!v.isString()) {
        failType(path, "字串");
    }
    return v.toString();
}

// JSON 數字是 double；只接受能精確表示的整數
std::int64_t asInteger(const QJsonValue& v, const QString& path) {
    constexpr double kMaxExact = 9007199254740992.0;   // 2^53
    if (!v.isDouble()) {
        failType(path, "整數");
    }
    const double d = v.toDouble();
    if (std::trunc(d) != d || std::fabs(d) > kMaxExact) {
        failType(path, "整數");
    }
    return static_cast<std::int64_t>(d);
}

int asInt(const QJsonValue& v, const QString& path) {
    const std::int64_t n = asInteger(v, path);
    if (n < INT_MIN || n > INT_MAX) {
        fail(QStringLiteral("欄位 %1 的值超出範圍").arg(path));
    }
    return static_cast<int>(n);
}

std::int64_t asNonNegative(const QJsonValue& v, const QString& path) {
    const std::int64_t n = asInteger(v, path);
    if (n < 0) {
        fail(QStringLiteral("欄位 %1 不可為負數").arg(path));
    }
    return n;
}

template <typename E, std::size_t N>
E asEnum(const QJsonValue& v, const QString& path, const NameTable<E, N>& table) {
    const QString s = asString(v, path);
    for (const auto& [name, e] : table) {
        if (s == QLatin1String(name)) {
            return e;
        }
    }
    fail(QStringLiteral("欄位 %1 的值「%2」無法辨識").arg(path, s));
}

core::PlayerInfo playerFromJson(const QJsonObject& players, const QString& key) {
    const QString path = "players." + key;
    const QJsonObject o = asObject(require(players, key, path), path);
    core::PlayerInfo p{asEnum(require(o, "type", path + ".type"), path + ".type", kPlayerTypes), std::nullopt};
    if (p.type == PlayerType::Ai) {
        p.difficulty = asEnum(require(o, "difficulty", path + ".difficulty"), path + ".difficulty", kDifficulties);
    }
    return p;
}

// players 須與 matchType 一致（SDD §6.1）
void checkPlayersMatch(MatchType t, const core::PlayerInfo& black, const core::PlayerInfo& white) {
    const int aiCount = (black.type == PlayerType::Ai) + (white.type == PlayerType::Ai);
    int expected = 0;
    switch (t) {
        case MatchType::Local:
        case MatchType::Lan:
            expected = 0;
            break;
        case MatchType::HumanVsAi:
            expected = 1;
            break;
        case MatchType::AiVsAi:
            expected = 2;
            break;
    }
    if (aiCount != expected) {
        fail(QStringLiteral("players 與 matchType「%1」不一致").arg(nameOf(t, kMatchTypes)));
    }
}

core::TimeControl timeControlFromJson(const QJsonObject& root) {
    const QJsonObject o = asObject(require(root, "timeControl", "timeControl"), "timeControl");
    const QString mode = asString(require(o, "mode", "timeControl.mode"), "timeControl.mode");
    if (mode == "none") {
        return {false, 0};
    }
    if (mode == "per_move") {
        const QString path = "timeControl.moveLimitMs";
        const std::int64_t limit = asInteger(require(o, "moveLimitMs", path), path);
        if (limit < kMinMoveLimitMs || limit > kMaxMoveLimitMs) {
            fail(QStringLiteral("欄位 %1 必須介於 %2 與 %3 之間").arg(path).arg(kMinMoveLimitMs).arg(kMaxMoveLimitMs));
        }
        return {true, limit};
    }
    fail(QStringLiteral("欄位 timeControl.mode 的值「%1」無法辨識").arg(mode));
}

std::vector<core::Move> movesFromJson(const QJsonObject& root) {
    const QJsonArray arr = asArray(require(root, "moves", "moves"), "moves");
    std::vector<core::Move> moves;
    core::Stone color = core::Stone::Black;
    for (qsizetype i = 0; i < arr.size(); ++i) {
        const QString path = QStringLiteral("moves[%1]").arg(i);
        const QJsonObject m = asObject(arr[i], path);
        const int r = asInt(require(m, "r", path + ".r"), path + ".r");
        const int c = asInt(require(m, "c", path + ".c"), path + ".c");
        const std::int64_t t = asNonNegative(require(m, "t", path + ".t"), path + ".t");
        moves.push_back({{r, c}, color, t});
        color = core::opponent(color);
    }
    return moves;
}

std::pair<GameResult, ResultReason> resultFromJson(const QJsonObject& root) {
    const QJsonValue v = require(root, "result", "result");
    if (v.isNull()) {
        return {GameResult::Ongoing, ResultReason::None};
    }
    const QJsonObject o = asObject(v, "result");
    const QJsonValue winner = require(o, "winner", "result.winner");
    GameResult r = GameResult::Draw;
    if (!winner.isNull()) {
        const QString w = asString(winner, "result.winner");
        if (w == "black") {
            r = GameResult::BlackWin;
        } else if (w == "white") {
            r = GameResult::WhiteWin;
        } else {
            fail(QStringLiteral("欄位 result.winner 的值「%1」無法辨識").arg(w));
        }
    }
    return {r, asEnum(require(o, "reason", "result.reason"), "result.reason", kReasons)};
}

core::GameRecord recordFromJson(const QJsonObject& root) {
    if (asString(require(root, "format", "format"), "format") != kFormatName) {
        fail(QStringLiteral("不是五子棋存檔（format 不是 \"%1\"）").arg(kFormatName));
    }
    const std::int64_t version = asInteger(require(root, "version", "version"), "version");
    if (version != kVersion) {
        fail(QStringLiteral("不支援的存檔版本 %1（目前支援 %2）").arg(version).arg(kVersion));
    }

    core::GameRecord r;
    const QString createdAt = asString(require(root, "createdAt", "createdAt"), "createdAt");
    const QDateTime dt = QDateTime::fromString(createdAt, Qt::ISODate);
    if (!dt.isValid() || dt.timeSpec() == Qt::LocalTime) {
        fail(QStringLiteral("欄位 createdAt 不是含時區的 ISO 8601 時間"));
    }
    r.createdAt = createdAt.toStdString();
    r.matchType = asEnum(require(root, "matchType", "matchType"), "matchType", kMatchTypes);

    const QJsonObject players = asObject(require(root, "players", "players"), "players");
    r.black = playerFromJson(players, "black");
    r.white = playerFromJson(players, "white");
    checkPlayersMatch(r.matchType, r.black, r.white);

    r.timeControl = timeControlFromJson(root);
    r.moves = movesFromJson(root);
    std::tie(r.result, r.reason) = resultFromJson(root);

    // 只有限時且未結束時需要（SDD §6.1），其他情況忽略
    if (r.timeControl.timed && r.result == GameResult::Ongoing) {
        const QJsonObject current = asObject(require(root, "current", "current"), "current");
        r.remainingMs = asNonNegative(require(current, "remainingMs", "current.remainingMs"), "current.remainingMs");
    }
    return r;
}

} // namespace

QByteArray toJson(const core::GameRecord& r) {
    QJsonObject timeControl{{"mode", r.timeControl.timed ? "per_move" : "none"}};
    if (r.timeControl.timed) {
        timeControl["moveLimitMs"] = static_cast<qint64>(r.timeControl.moveLimitMs);
    }

    QJsonArray moves;
    for (const core::Move& m : r.moves) {
        moves.append(QJsonObject{{"r", m.pos.row}, {"c", m.pos.col}, {"t", static_cast<qint64>(m.timeUsedMs)}});
    }

    QJsonObject root{
        {"format", kFormatName},
        {"version", kVersion},
        {"createdAt", QString::fromStdString(r.createdAt)},
        {"matchType", nameOf(r.matchType, kMatchTypes)},
        {"players", QJsonObject{{"black", playerToJson(r.black)}, {"white", playerToJson(r.white)}}},
        {"timeControl", timeControl},
        {"moves", moves},
        {"result", resultToJson(r.result, r.reason)},
    };
    if (r.timeControl.timed && r.result == GameResult::Ongoing && r.remainingMs) {
        root["current"] = QJsonObject{{"remainingMs", static_cast<qint64>(*r.remainingMs)}};
    }
    return QJsonDocument(root).toJson(QJsonDocument::Indented);
}

std::variant<core::GameRecord, QString> fromJson(const QByteArray& bytes) {
    QJsonParseError parseError;
    const QJsonDocument doc = QJsonDocument::fromJson(bytes, &parseError);
    if (parseError.error != QJsonParseError::NoError) {
        return QStringLiteral("不是有效的 JSON：%1").arg(parseError.errorString());
    }
    if (!doc.isObject()) {
        return QStringLiteral("檔案內容不是 JSON 物件");
    }
    try {
        return recordFromJson(doc.object());
    } catch (const FormatError& e) {
        return e.message;
    }
}

} // namespace SaveFormat
