#pragma once

#include <variant>

#include <QByteArray>
#include <QString>

#include "GameRecord.h"

// 存檔／棋譜與 JSON 互轉（SDD §5.6、§6.1）。
// 這裡只檢查欄位是否齊全、型別與值是否正確；棋步合法性與勝負交給 core::restore 驗證。
namespace SaveFormat {

QByteArray toJson(const core::GameRecord& r);
std::variant<core::GameRecord, QString> fromJson(const QByteArray& bytes);

} // namespace SaveFormat
