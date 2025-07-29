#include <QApplication>
#include <QSurfaceFormat>

#include "ui/MainWindow.h"
int main(int argc, char *argv[]) {
    QSurfaceFormat fmt;
    fmt.setVersion(4, 1);                     // 또는 (4,1)
    fmt.setProfile(QSurfaceFormat::CoreProfile);
    fmt.setDepthBufferSize(24);
    QSurfaceFormat::setDefaultFormat(fmt);    // ★ 모든 위젯에 적용

    QApplication app(argc, argv);

    MainWindow win;
    win.resize(1200, 800);
    win.show();


    return QApplication::exec();
}
