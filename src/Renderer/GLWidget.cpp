#include "GLWidget.h"
#include <QTimer>
#include <string>

QVector3D eye = {1, 1, 1};

GLWidget::GLWidget(QWidget* parent)
    : QOpenGLWidget(parent) {}


void GLWidget::initializeGL() {
    initializeOpenGLFunctions();

    glEnable(GL_DEPTH_TEST);

    loadShaders(":/shaders/phong.vert", ":/shaders/phong.frag");
    loadModel("./res/models/teddybear.obj"); // 컴파일된 실행파일 기준 - cmake-build-debug directory

    view_.lookAt(eye, {0,0,0}, {0,1,0});
    modelMat_.setToIdentity();

    QVector3D cen = {model_.center().x, model_.center().y, model_.center().z};

    modelMat_.translate(-cen); // 원점으로
    float unitScale = 1.0f / model_.maxExtent();
    modelMat_.scale(unitScale);

    glClearColor(0.0f, 0.5f, 0.1f, 1.0f);
    //glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
}

void GLWidget::resizeGL(int w, int h) {
    glViewport(0, 0, w, h);
    proj_.setToIdentity();
    proj_.perspective(45.0f, static_cast<float>(w)/h, 0.1f, 100.0f);
}

void GLWidget::paintGL() {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    shader_.bind();
    shader_.setUniformValue("uProj",  proj_);
    shader_.setUniformValue("uView",  view_);
    shader_.setUniformValue("uModel", modelMat_);
    
    shader_.setUniformValue("uLightPos", QVector3D(3,3,0));
    shader_.setUniformValue("uViewPos",  eye);

    glBindVertexArray(vao_);
    glDrawElements(GL_TRIANGLES,
                   static_cast<GLsizei>(model_.indices().size()),
                   GL_UNSIGNED_INT,
                   nullptr);
    glBindVertexArray(0);
    shader_.release();
}

void GLWidget::loadShaders(const QString& vert, const QString& frag) {
    shader_.addShaderFromSourceFile(QOpenGLShader::Vertex, vert);
    shader_.addShaderFromSourceFile(QOpenGLShader::Fragment,frag);
    shader_.link();
}

void GLWidget::loadModel(const QString& path) {
    if (!model_.load(path.toStdString()))
        throw std::runtime_error("OBJ load failed");

    const auto& verts   = model_.vertices();
    const auto& indices = model_.indices();

    glGenVertexArrays(1, &vao_);
    glGenBuffers(1, &vbo_);
    glGenBuffers(1, &ebo_);

    glBindVertexArray(vao_);
    glBindBuffer(GL_ARRAY_BUFFER, vbo_);
    glBufferData(GL_ARRAY_BUFFER,
                 verts.size() * sizeof(Vertex),
                 verts.data(),
                 GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo_);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER,
                 indices.size() * sizeof(uint32_t),
                 indices.data(),
                 GL_STATIC_DRAW);

    std::size_t offPos = offsetof(Vertex, position);
    std::size_t offNrm = offsetof(Vertex, normal);
    std::size_t offUV  = offsetof(Vertex, texcoord);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                          reinterpret_cast<void*>(offPos));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                          reinterpret_cast<void*>(offNrm));
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                          reinterpret_cast<void*>(offUV));

    glBindVertexArray(0);
}

