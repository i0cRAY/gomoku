#include "MainWindow.h"

MainWindow::MainWindow(QWidget* parent) : QMainWindow(parent) {
    resize(640, 640);
    setWindowTitle(tr("Gomoku"));
}
