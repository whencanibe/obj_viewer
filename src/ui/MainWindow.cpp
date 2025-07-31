#include "MainWindow.h"
#include "../Renderer/GLWidget.h"
#include <QDockWidget>
#include <QRadioButton>
#include <QLabel>
#include <QHBoxLayout>
#include <QButtonGroup>
#include <QStatusBar>
#include <QSlider>
#include <QDoubleSpinBox>

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

    // Lighting
    auto* dockL = new QDockWidget("Lighting", this);
    auto* paneL = new QWidget;
    auto* vl = new QVBoxLayout(paneL);

    /* --- Light orbit sliders --- */
    auto* yawSlider   = new QSlider(Qt::Horizontal); yawSlider  ->setRange(0,360);
    auto* pitchSlider = new QSlider(Qt::Horizontal); pitchSlider->setRange(-89,89);
    auto* distSpin    = new QDoubleSpinBox; distSpin->setRange(0.5,20); distSpin->setSingleStep(0.1);
    distSpin->setValue(3.0);

    vl->addWidget(new QLabel("Yaw"));   vl->addWidget(yawSlider);
    vl->addWidget(new QLabel("Pitch")); vl->addWidget(pitchSlider);
    vl->addWidget(new QLabel("Distance")); vl->addWidget(distSpin);

    /* --- Phong coefficients --- */
    auto* kdSlider  = new QSlider(Qt::Horizontal); kdSlider ->setRange(0,100); kdSlider ->setValue(100);
    auto* ksSlider  = new QSlider(Qt::Horizontal); ksSlider ->setRange(0,100); ksSlider ->setValue(40);
    auto* shinSpin  = new QSpinBox; shinSpin->setRange(1,128); shinSpin->setValue(32);

    vl->addWidget(new QLabel("Diffuse (Kd)"));   vl->addWidget(kdSlider);
    vl->addWidget(new QLabel("Specular (Ks)"));  vl->addWidget(ksSlider);
    vl->addWidget(new QLabel("Shininess"));      vl->addWidget(shinSpin);

    paneL->setLayout(vl);  dockL->setWidget(paneL);
    addDockWidget(Qt::RightDockWidgetArea, dockL);

    /* signal-slot 연결 */
    connect(yawSlider,   &QSlider::valueChanged,      glWidget_, &GLWidget::setLightYaw);
    connect(pitchSlider, &QSlider::valueChanged,      glWidget_, &GLWidget::setLightPitch);
    connect(distSpin,    QOverload<double>::of(&QDoubleSpinBox::valueChanged),
                         glWidget_, &GLWidget::setLightRadius);
    connect(kdSlider,    &QSlider::valueChanged,      glWidget_, &GLWidget::setKd);
    connect(ksSlider,    &QSlider::valueChanged,      glWidget_, &GLWidget::setKs);
    connect(shinSpin,    QOverload<int>::of(&QSpinBox::valueChanged),
                         glWidget_, &GLWidget::setShininess);


    // 상태바 등 추가도 가능
    //statusBar()->showMessage("Ready", 3000);

    qDebug() << "glWidget_ =" << glWidget_;
}
