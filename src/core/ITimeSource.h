#pragma once

#include <cstdint>

namespace core {

// 時間來源介面（SDD §5.1）。core 只定義介面，不讀系統時間；
// 測試使用假時鐘，實際讀時間的實作放在 app 層。
class ITimeSource {
public:
    virtual ~ITimeSource() = default;
    virtual std::int64_t nowMs() const = 0;   // 單調遞增的毫秒數，起點不限
};

} // namespace core
