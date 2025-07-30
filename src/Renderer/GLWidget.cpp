#include "GLWidget.h"
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

    loadShaders(":/shaders/phong.vert", ":/shaders/phong.frag");
    loadModel("./res/models/teddybear.obj"); // 컴파일된 실행파일 기준 - cmake-build-debug directory

    camDist_ = 5.5f;
    updateCamera();

    setModelMat();

    glClearColor(0.0f, 0.5f, 0.1f, 1.0f);
    //glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
}

void GLWidget::resizeGL(int w, int h) {
    glViewport(0, 0, w, h);
    proj_.setToIdentity();
    proj_.perspective(45.0f, static_cast<float>(w) / h, 0.1f, 100.0f);
}

void GLWidget::paintGL() {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    shader_.bind();
    shader_.setUniformValue("uProj", proj_);
    shader_.setUniformValue("uView", view_);
    shader_.setUniformValue("uModel", modelMat_);

    shader_.setUniformValue("uLightPos", QVector3D(3, 3, 0));
    shader_.setUniformValue("uViewPos", eye_);

    glBindVertexArray(vao_);
    glDrawElements(GL_TRIANGLES,
                   static_cast<GLsizei>(model_.indices().size()),
                   GL_UNSIGNED_INT,
                   nullptr);
    glBindVertexArray(0);
    shader_.release();

    GLenum err = glGetError();
    if (err != GL_NO_ERROR)
        qDebug() << "GL ERROR =" << err;
}

void GLWidget::loadShaders(const QString &vert, const QString &frag) {
    shader_.addShaderFromSourceFile(QOpenGLShader::Vertex, vert);
    shader_.addShaderFromSourceFile(QOpenGLShader::Fragment, frag);
    shader_.link();
}

bool GLWidget::loadModel(const QString &path) {
    if (!model_.load(path.toStdString()))
        return false; // ① OBJ, 노멀, AABB 등 계산

    if (!vao_) {
        // ② VAO·VBO·EBO 한 번만 만들기
        glGenVertexArrays(1, &vao_);
        glGenBuffers(1, &vbo_);
        glGenBuffers(1, &ebo_);

        glBindVertexArray(vao_);
        glBindBuffer(GL_ARRAY_BUFFER, vbo_);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo_);

        // attribute 포인터 고정 (위치·노멀·UV)
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3,GL_FLOAT,GL_FALSE, sizeof(Vertex), (void *) offsetof(Vertex, position));
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 3,GL_FLOAT,GL_FALSE, sizeof(Vertex), (void *) offsetof(Vertex, normal));
        glEnableVertexAttribArray(2);
        glVertexAttribPointer(2, 2,GL_FLOAT,GL_FALSE, sizeof(Vertex), (void *) offsetof(Vertex, texcoord));

        glBindVertexArray(0);
    }

    uploadVertexBuffer(); // ③ 데이터(GPU) 전송은 별도 함수

    setModelMat();

    return true;
}

void GLWidget::uploadVertexBuffer() {
    const auto &verts = model_.vertices();
    const auto &idx = model_.indices();

    glBindVertexArray(vao_);

    glBindBuffer(GL_ARRAY_BUFFER, vbo_);
    glBufferData(GL_ARRAY_BUFFER,
                 verts.size() * sizeof(Vertex),
                 verts.data(),
                 GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo_);
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
    // ── 여기서 매트릭스 갱신 ──
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

    /* zoomFactor <1 → 줌인,  >1 → 줌아웃 */
    float zoomFactor = (numDegrees > 0) ? 0.9f : 1.1f;
    camDist_ *= zoomFactor;
    camDist_ = std::clamp(camDist_, 0.8f, 20.0f);

    /* eye = 카메라축 * camDist */
    //QVector3D dir = (eye_ - target_).normalized(); // 현재 바라보는 방향 유지
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

    const float sens = 0.5f;          // 감도
    yaw_   -= delta.x() * sens;
    pitch_ += delta.y() * sens;
    pitch_  = std::clamp(pitch_, -89.f, 89.f);

    updateCamera();
    update();                         // repaint
}

void GLWidget::updateCamera()
{
    /* 1. 회전 쿼터니언 만들기 (World‑up → Local‑right 순) */
    QQuaternion qYaw   = QQuaternion::fromAxisAndAngle({0,1,0}, yaw_);
    QQuaternion qPitch = QQuaternion::fromAxisAndAngle({1,0,0}, pitch_);
    QQuaternion rot    = qYaw * qPitch;           //  순서 중요

    /* 2. 기본 전방 벡터(0,0,1)에 회전 적용 → dir */
    QVector3D dir = rot.rotatedVector({0,0,1}).normalized();

    /* 3. eye = target + dir * radius */
    eye_ = target_ + dir * camDist_;

    /* 4. view 행렬 */
    view_.setToIdentity();
    view_.lookAt(eye_, target_, {0,1,0});
}
