#pragma once

#include <cstdint>

#include "ITimeSource.h"

// 測試用假時鐘：時間只在測試呼叫 advance / set 時改變，
// 或設定 autoAdvanceMs 讓每次讀取後自動前進，讓時間上限的測試完全可預測。
class FakeTimeSource : public core::ITimeSource {
public:
    explicit FakeTimeSource(std::int64_t startMs = 0, std::int64_t autoAdvanceMs = 0)
        : m_now(startMs), m_autoAdvance(autoAdvanceMs) {}

    std::int64_t nowMs() const override {
        const std::int64_t t = m_now;
        m_now += m_autoAdvance;
        ++m_reads;
        return t;
    }

    void advance(std::int64_t ms) { m_now += ms; }
    void set(std::int64_t ms) { m_now = ms; }
    void setAutoAdvance(std::int64_t ms) { m_autoAdvance = ms; }
    int reads() const { return m_reads; }

private:
    mutable std::int64_t m_now;
    std::int64_t m_autoAdvance;
    mutable int m_reads = 0;
};
