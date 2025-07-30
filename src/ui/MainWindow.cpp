#include "MainWindow.h"
#include "../Renderer/GLWidget.h"
#include <QDockWidget>
#include <QRadioButton>
#include <QLabel>
#include <QHBoxLayout>
#include <QButtonGroup>
#include <QStatusBar>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent) {
    // 중앙
    glWidget_ = new GLWidget(this);
    setCentralWidget(glWidget_);

    auto *dock = new QDockWidget("Controls", this);
    dock->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);

    /* 패널 & 전체 세로 레이아웃 */
    auto* panel  = new QWidget;
    auto* vbox   = new QVBoxLayout(panel);

    auto* normalModeTitle = new QLabel("<b>Normal&nbsp;Mode</b>");
    vbox->addWidget(normalModeTitle);

    /* 노멀 모드 선택 : 가로 배치 */
    auto* hbox       = new QHBoxLayout;
    auto* radioVert  = new QRadioButton("Vertex");
    auto* radioFace  = new QRadioButton("Face");
    radioVert->setChecked(true);

    hbox->addWidget(radioVert);
    hbox->addWidget(radioFace);
    hbox->addStretch();                   // 오른쪽 여백

    vbox->addLayout(hbox);                // 세로 레이아웃에 삽입
    vbox->addStretch();                   // 아래쪽 빈 공간

    panel->setLayout(vbox);

    dock->setWidget(panel);
    addDockWidget(Qt::RightDockWidgetArea, dock);

    /* 버튼 그룹 & 시그널 연결 */
    auto* group = new QButtonGroup(panel);
    group->addButton(radioVert, 0);
    group->addButton(radioFace, 1);

    connect(group, QOverload<int>::of(&QButtonGroup::idClicked),
            this, [this](int id){
        auto mode = (id == 0) ? NormalMode::Vertex : NormalMode::Face;
        glWidget_->setNormalMode(mode);   // GLWidget 내부에서 rebuild + GPU 업로드
    });



    // 상태바 등 추가도 가능
    statusBar()->showMessage("Ready", 3000);

    qDebug() << "glWidget_ =" << glWidget_;
}
