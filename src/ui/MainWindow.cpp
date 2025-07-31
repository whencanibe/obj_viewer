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

    // 최상위 패널
    auto *mainPanel = new QWidget;
    auto *mainLayout = new QVBoxLayout(mainPanel);

    /* 패널 & 전체 세로 레이아웃 */
    auto *modelGroup = new QWidget;
    auto *modelLayout = new QVBoxLayout(modelGroup);

    modelLayout->addWidget(new QLabel("<b>Normal&nbsp;Mode</b>"));

    /* 노멀 모드 선택 : 가로 배치 */
    auto *hbox = new QHBoxLayout;
    auto *radioVert = new QRadioButton("Vertex");
    auto *radioFace = new QRadioButton("Face");
    radioVert->setChecked(true);

    hbox->addWidget(radioVert);
    hbox->addWidget(radioFace);
    hbox->addStretch(); // 오른쪽 여백

    modelLayout->addLayout(hbox); // 세로 레이아웃에 삽입
    modelLayout->addStretch(); // 아래쪽 빈 공간

    /* 버튼 그룹 & 시그널 연결 */
    auto *group = new QButtonGroup(modelGroup);
    group->addButton(radioVert, 0);
    group->addButton(radioFace, 1);

    // Lighting
    auto *lightingGroup = new QWidget;
    auto *lightingLayout = new QVBoxLayout(lightingGroup);
    lightingLayout->addWidget(new QLabel("<b>Lighting</b>"));

    /* --- Light orbit sliders --- */
    auto *yawSlider = new QSlider(Qt::Horizontal);
    yawSlider->setRange(0, 360);
    yawSlider->setValue(45);
    auto *pitchSlider = new QSlider(Qt::Horizontal);
    pitchSlider->setRange(-89, 89);
    pitchSlider->setValue(30);

    auto *distSpin = new QDoubleSpinBox;
    distSpin->setRange(0.5, 20);
    distSpin->setSingleStep(0.1);
    distSpin->setValue(2.0);

    lightingLayout->addWidget(new QLabel("Yaw"));
    lightingLayout->addWidget(yawSlider);
    lightingLayout->addWidget(new QLabel("Pitch"));
    lightingLayout->addWidget(pitchSlider);
    lightingLayout->addWidget(new QLabel("Distance"));
    lightingLayout->addWidget(distSpin);

    /* --- Phong coefficients --- */
    auto *kdSlider = new QSlider(Qt::Horizontal);
    kdSlider->setRange(0, 100);
    kdSlider->setValue(100);
    auto *ksSlider = new QSlider(Qt::Horizontal);
    ksSlider->setRange(0, 100);
    ksSlider->setValue(40);
    auto *shinSpin = new QSpinBox;
    shinSpin->setRange(1, 128);
    shinSpin->setValue(32);

    lightingLayout->addWidget(new QLabel("Diffuse (Kd)"));
    lightingLayout->addWidget(kdSlider);
    lightingLayout->addWidget(new QLabel("Specular (Ks)"));
    lightingLayout->addWidget(ksSlider);
    lightingLayout->addWidget(new QLabel("Shininess"));
    lightingLayout->addWidget(shinSpin);

    mainLayout->addWidget(modelGroup);
    mainLayout->addWidget(lightingGroup);

    dock->setWidget(mainPanel);
    addDockWidget(Qt::RightDockWidgetArea, dock);

    connect(group, QOverload<int>::of(&QButtonGroup::idClicked),
            this, [this](int id) {
                auto mode = (id == 0) ? NormalMode::Vertex : NormalMode::Face;
                glWidget_->setNormalMode(mode); // GLWidget 내부에서 rebuild + GPU 업로드
            });

    /* signal-slot 연결 */
    connect(yawSlider, &QSlider::valueChanged, glWidget_, &GLWidget::setLightYaw);
    connect(pitchSlider, &QSlider::valueChanged, glWidget_, &GLWidget::setLightPitch);
    connect(distSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
            glWidget_, &GLWidget::setLightRadius);
    connect(kdSlider, &QSlider::valueChanged, glWidget_, &GLWidget::setKd);
    connect(ksSlider, &QSlider::valueChanged, glWidget_, &GLWidget::setKs);
    connect(shinSpin, QOverload<int>::of(&QSpinBox::valueChanged),
            glWidget_, &GLWidget::setShininess);


    // 상태바 등 추가도 가능
    //statusBar()->showMessage("Ready", 3000);

    qDebug() << "glWidget_ =" << glWidget_;
}
