#include "GLWidget.h"
#include <string>

GLWidget::GLWidget(QWidget *parent)
    : QOpenGLWidget(parent) {
    setFocusPolicy(Qt::StrongFocus); // 위젯이 키보드 포커스 받을 수 있도록
    connect(&timer_, &QTimer::timeout, this, QOverload<>::of(&GLWidget::update));
    timer_.start(16); // ~60 FPS
}


void GLWidget::initializeGL() {
    initializeOpenGLFunctions();

    glEnable(GL_DEPTH_TEST);

    loadShaders(phongProg_, ":/shaders/phong.vert", ":/shaders/phong.frag");
    loadShaders(gridProg_, ":/shaders/grid.vert", ":/shaders/grid.frag");

    loadModel("./res/models/teddybear.obj"); // 컴파일된 실행파일 기준 - cmake-build-debug directory
    loadCube();

    camDist_ = 3.5f;
    updateCamera();
    updateLight();
    setModelMat();

    glClearColor(107 / 255.f, 142 / 255.f, 35 / 255.f, 1.0f);
}

void GLWidget::resizeGL(int w, int h) {
    glViewport(0, 0, w, h);
    proj_.setToIdentity();
    proj_.perspective(45.0f, static_cast<float>(w) / h, 0.1f, 100.0f);
}

void GLWidget::paintGL() {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    drawGrid();
    drawModel();
    drawLight();

    GLenum err = glGetError();
    if (err != GL_NO_ERROR)
        qDebug() << "GL ERROR =" << err;
}

void GLWidget::loadShaders(QOpenGLShaderProgram &program, const QString &vert, const QString &frag) {
    program.addShaderFromSourceFile(QOpenGLShader::Vertex, vert);
    program.addShaderFromSourceFile(QOpenGLShader::Fragment, frag);
    program.link();
}

bool GLWidget::loadModel(const QString &path) {
    if (!model_.load(path.toStdString()))
        return false;

    if (!vaoModel_) {
        // ② VAO·VBO·EBO 한 번만 만들기
        glGenVertexArrays(1, &vaoModel_);
        glGenBuffers(1, &vboModel_);
        glGenBuffers(1, &eboModel_);

        glBindVertexArray(vaoModel_);
        glBindBuffer(GL_ARRAY_BUFFER, vboModel_);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, eboModel_);

        // attribute 포인터 고정 (위치·노멀·UV)
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3,GL_FLOAT,GL_FALSE, sizeof(Vertex), (void *) offsetof(Vertex, position));
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 3,GL_FLOAT,GL_FALSE, sizeof(Vertex), (void *) offsetof(Vertex, normal));
        glEnableVertexAttribArray(2);
        glVertexAttribPointer(2, 2,GL_FLOAT,GL_FALSE, sizeof(Vertex), (void *) offsetof(Vertex, texcoord));

        glBindVertexArray(0);
    }

    uploadVertexBuffer();

    setModelMat();

    return true;
}

void GLWidget::uploadVertexBuffer() {
    const auto &verts = model_.vertices();
    const auto &idx = model_.indices();

    glBindVertexArray(vaoModel_);

    glBindBuffer(GL_ARRAY_BUFFER, vboModel_);
    glBufferData(GL_ARRAY_BUFFER,
                 verts.size() * sizeof(Vertex),
                 verts.data(),
                 GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, eboModel_);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER,
                 idx.size() * sizeof(uint32_t),
                 idx.data(),
                 GL_STATIC_DRAW);

    glBindVertexArray(0);

    qDebug() << "verts =" << model_.vertices().size()
            << "idx   =" << model_.indices().size();
}

void GLWidget::toggleNormalMode() {
    auto next = (model_.normalMode() == NormalMode::Vertex)
                    ? NormalMode::Face
                    : NormalMode::Vertex;
    model_.setNormalMode(next); // CPU 쪽 vertices_/indices_ 재조립
    uploadVertexBuffer(); // GPU 버퍼 다시 채워주기
    update(); // repaint
}

void GLWidget::setNormalMode(NormalMode mode) {
    if (model_.normalMode() == mode)
        return;

    model_.setNormalMode(mode);

    makeCurrent(); // 컨텍스트 활성
    uploadVertexBuffer();
    doneCurrent(); // 컨텍스트 반환

    update();
}

void GLWidget::setModelMat() {
    modelMat_.setToIdentity();
    float s = 1.0f / model_.maxExtent();
    modelMat_.scale(s);
    modelMat_.translate(-QVector3D(model_.center().x, // 원점 이동
                                   model_.center().y,
                                   model_.center().z));
}

void GLWidget::keyPressEvent(QKeyEvent *e) {
    if (e->key() == Qt::Key_N) {
        toggleNormalMode();
    } else {
        QOpenGLWidget::keyPressEvent(e); // 다른 키는 기본 처리
    }
}

void GLWidget::wheelEvent(QWheelEvent *event) {
    float numDegrees = event->angleDelta().y() / 8.0f;
    if (numDegrees == 0) {
        event->accept();
        return;
    }

    float zoomFactor = (numDegrees > 0) ? 0.9f : 1.1f;
    camDist_ *= zoomFactor;
    camDist_ = std::clamp(camDist_, 0.8f, 20.0f);

    updateCamera();

    update(); // paintGL() 재호출
    event->accept();
}

void GLWidget::mousePressEvent(QMouseEvent *e) {
    if (e->button() == Qt::LeftButton)
        lastMousePos_ = e->pos();
}

void GLWidget::mouseMoveEvent(QMouseEvent *e) {
    if (!(e->buttons() & Qt::LeftButton)) {
        QOpenGLWidget::mouseMoveEvent(e);
        return;
    }

    QPoint delta = e->pos() - lastMousePos_;
    lastMousePos_ = e->pos();

    const float sens = 0.5f; // 감도
    yaw_ -= delta.x() * sens;
    pitch_ += delta.y() * sens;
    pitch_ = std::clamp(pitch_, -89.f, 89.f);

    updateCamera();
    update(); // repaint
}

void GLWidget::updateCamera() {
    QQuaternion qYaw = QQuaternion::fromAxisAndAngle({0, 1, 0}, yaw_);
    QQuaternion qPitch = QQuaternion::fromAxisAndAngle({1, 0, 0}, pitch_);
    QQuaternion rot = qYaw * qPitch; //  순서 중요

    QVector3D dir = rot.rotatedVector({0, 0, 1}).normalized();

    eye_ = target_ + dir * camDist_;

    view_.setToIdentity();
    view_.lookAt(eye_, target_, {0, 1, 0});
}

// 버튼 토글 위하여
void GLWidget::setShowGrid(bool on) {
    showGrid_ = on;
    update();
}

void GLWidget::loadCube() {
    cube_.load("./res/models/cube.obj");

    glGenVertexArrays(1, &vaoGrid_);
    glGenBuffers(1, &vboGrid_);
    glGenBuffers(1, &eboGrid_);

    glBindVertexArray(vaoGrid_);
    glBindBuffer(GL_ARRAY_BUFFER, vboGrid_);
    glBufferData(GL_ARRAY_BUFFER,
                 cube_.vertices().size() * sizeof(Vertex),
                 cube_.vertices().data(), GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, eboGrid_);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER,
                 cube_.indices().size() * sizeof(uint32_t),
                 cube_.indices().data(), GL_STATIC_DRAW);

    glEnableVertexAttribArray(0); // 위치만 있으면 OK
    glVertexAttribPointer(0, 3,GL_FLOAT,GL_FALSE, sizeof(Vertex),
                          (void *) offsetof(Vertex, position));
    glBindVertexArray(0);
}

void GLWidget::drawModel() {
    phongProg_.bind();
    phongProg_.setUniformValue("uProj", proj_);
    phongProg_.setUniformValue("uView", view_);
    phongProg_.setUniformValue("uModel", modelMat_);
    phongProg_.setUniformValue("uViewPos", eye_);

    phongProg_.setUniformValue("uLightPos", lightPos_);
    phongProg_.setUniformValue("uKd", kd_);
    phongProg_.setUniformValue("uKs", ks_);
    phongProg_.setUniformValue("uShin", shininess_);

    glBindVertexArray(vaoModel_);
    glDrawElements(GL_TRIANGLES,
                   model_.indices().size(),
                   GL_UNSIGNED_INT, nullptr);
    glBindVertexArray(0);
    phongProg_.release();
}

void GLWidget::drawGrid() {
    if (!showGrid_) return;

    gridProg_.bind();
    gridProg_.setUniformValue("uColor", QVector4D(1, 1, 1, 1));

    QMatrix4x4 PV = proj_ * view_;
    const int n = 5000;
    const float size = model_.maxExtent(); // 모델 크기가 다 다르기 때문에
    const float len = size * 1000;
    const float step = len / n;
    const float thickness = 0.003f;


    glBindVertexArray(vaoGrid_);
    for (int j = 0; j <= n; ++j) {
        float z = -len / 2 + step * j;
        QMatrix4x4 M;

        //M.translate(QVector3D(-gridCube_.center().x, -gridCube_.center().y, -gridCube_.center().z));
        //M.translate(0, -0.05f * size, z);

        M.translate(-len / 2.f, -0.05f * size, z);
        M.scale(len, thickness * size, thickness * size);

        gridProg_.setUniformValue("uMVP", PV * M);
        glDrawElements(GL_TRIANGLES, cube_.indices().size(),
                       GL_UNSIGNED_INT, nullptr);
    }
    for (int j = 0; j <= n; ++j) {
        float x = -len / 2 + j * step;
        QMatrix4x4 M;

        M.translate(x, -0.05f * size, -len / 2.f);
        M.scale(thickness * size, thickness * size, len);
        gridProg_.setUniformValue("uMVP", PV * M);
        glDrawElements(GL_TRIANGLES, cube_.indices().size(),
                       GL_UNSIGNED_INT, nullptr);
    }
    glBindVertexArray(0);
    gridProg_.release();
}

void GLWidget::drawLight() {
    // 모델 크기에 맞춰서 빛 크기 조절
    float s = model_.maxExtent() * 0.005f;

    QMatrix4x4 M;

    M.translate(lightPos_);
    M.scale(s);
    M.translate(-QVector3D(cube_.center().x,
                           cube_.center().y,
                           cube_.center().z));

    gridProg_.bind();
    gridProg_.setUniformValue("uColor", QVector4D(1,1,1,1));
    gridProg_.setUniformValue("uMVP", proj_ * view_ * M);

    glBindVertexArray(vaoGrid_);
    glDrawElements(GL_TRIANGLES,
                   cube_.indices().size(),
                   GL_UNSIGNED_INT, nullptr);
    glBindVertexArray(0);
    gridProg_.release();
}

void GLWidget::setLightYaw(int deg) {
    lightYaw_ = deg;
    updateLight();
}

void GLWidget::setLightPitch(int deg) {
    lightPitch_ = deg;
    updateLight();
}

void GLWidget::setLightRadius(double r) {
    lightRadius_ = r;
    updateLight();
}

void GLWidget::updateLight() {
    float ry = qDegreesToRadians(lightYaw_);
    float rp = qDegreesToRadians(lightPitch_);

    QVector3D dir(cos(rp) * cos(ry),
                  sin(rp),
                  cos(rp) * sin(ry));

    lightPos_ = target_ + dir.normalized() * lightRadius_;
    update();
}


void GLWidget::setKd(int v) {
    kd_ = v / 100.0f;
    update();
}

void GLWidget::setKs(int v) {
    ks_ = v / 100.0f;
    update();
}

void GLWidget::setShininess(int v) {
    shininess_ = float(v);
    update();
}
