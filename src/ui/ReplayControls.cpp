#include "ReplayControls.h"

#include <QHBoxLayout>
#include <QPushButton>

ReplayControls::ReplayControls(QWidget* parent) : QWidget(parent) {
    auto* layout = new QHBoxLayout(this);
    auto addButton = [&](const QString& text, void (ReplayControls::*signal)()) {
        auto* b = new QPushButton(text, this);
        connect(b, &QPushButton::clicked, this, signal);
        layout->addWidget(b);
        return b;
    };
    m_toStart = addButton(tr("⏮ 開頭"), &ReplayControls::toStartRequested);
    m_prev = addButton(tr("◀ 上一步"), &ReplayControls::prevRequested);
    m_playPause = addButton(tr("▶ 播放"), &ReplayControls::playPauseRequested);
    m_next = addButton(tr("下一步 ▶"), &ReplayControls::nextRequested);
    m_toEnd = addButton(tr("結尾 ⏭"), &ReplayControls::toEndRequested);
    layout->addStretch();
    addButton(tr("結束回放"), &ReplayControls::exitRequested);
    updateButtons();
}

void ReplayControls::setPosition(std::size_t index, std::size_t size) {
    m_index = index;
    m_size = size;
    updateButtons();
}

void ReplayControls::setPlaying(bool playing) {
    m_playing = playing;
    updateButtons();
}

void ReplayControls::updateButtons() {
    const bool atStart = m_index == 0;
    const bool atEnd = m_index >= m_size;
    m_toStart->setEnabled(!atStart);
    m_prev->setEnabled(!atStart);
    m_next->setEnabled(!atEnd);
    m_toEnd->setEnabled(!atEnd);
    m_playPause->setText(m_playing ? tr("⏸ 暫停") : tr("▶ 播放"));
    m_playPause->setEnabled(m_playing || !atEnd);   // 已在最後一手時無法播放
}
