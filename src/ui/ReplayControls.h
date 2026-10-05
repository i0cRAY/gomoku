#pragma once

#include <cstddef>

#include <QWidget>

class QPushButton;

// 回放時的操作列（SDD §5.4）：開頭、上一步、播放／暫停、下一步、結尾、結束回放。
// 只負責按鈕與啟用狀態，實際移動由 MainWindow 處理。
class ReplayControls : public QWidget {
    Q_OBJECT

public:
    explicit ReplayControls(QWidget* parent = nullptr);

    void setPosition(std::size_t index, std::size_t size);
    void setPlaying(bool playing);

signals:
    void toStartRequested();
    void prevRequested();
    void playPauseRequested();
    void nextRequested();
    void toEndRequested();
    void exitRequested();

private:
    void updateButtons();

    QPushButton* m_toStart = nullptr;
    QPushButton* m_prev = nullptr;
    QPushButton* m_playPause = nullptr;
    QPushButton* m_next = nullptr;
    QPushButton* m_toEnd = nullptr;
    std::size_t m_index = 0;
    std::size_t m_size = 0;
    bool m_playing = false;
};
