#include "MainWindow.h"
#include "../Renderer/GLWidget.h"
#include <QStatusBar>

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent) {

    // OpenGL 위젯 생성
    auto* glWidget = new GLWidget(this);

    // 중앙에 위젯 붙이기
    setCentralWidget(glWidget);

    // 상태바 등 추가도 가능
    statusBar()->showMessage("Ready", 3000);
}

MainWindow::~MainWindow() = default;