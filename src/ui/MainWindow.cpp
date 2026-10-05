#include "MainWindow.h"

#include "BoardView.h"

MainWindow::MainWindow(QWidget* parent) : QMainWindow(parent) {
    resize(640, 640);
    setWindowTitle(tr("Gomoku"));
    setCentralWidget(new BoardView(this));
}
