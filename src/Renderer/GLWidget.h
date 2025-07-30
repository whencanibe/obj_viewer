#ifndef GLWIDGET_H
#define GLWIDGET_H

#include <QOpenGLWidget>
#include <QOpenGLFunctions>
#include <QOpenGLShaderProgram>
#include <QMatrix4x4>
#include <QTimer>
#include <QKeyEvent>

#include "../core/ModelLoader.h"

class GLWidget : public QOpenGLWidget, protected QOpenGLFunctions {
    Q_OBJECT

public:
    explicit GLWidget(QWidget *parent = nullptr);

    ~GLWidget() override = default;

public slots:
    void toggleNormalMode();

    void setNormalMode(NormalMode mode);

protected:
    void initializeGL() override;

    void resizeGL(int w, int h) override;

    void paintGL() override;

    void keyPressEvent(QKeyEvent *e) override;

    void wheelEvent(QWheelEvent *event) override;

    void mousePressEvent(QMouseEvent *e) override;

    void mouseMoveEvent(QMouseEvent *e) override;

private:
    bool loadModel(const QString &path);

    void loadShaders(const QString &vert, const QString &frag);

    void uploadVertexBuffer();

    void setModelMat();

    void updateCamera();

    QTimer timer_;

    ModelLoader model_;
    QOpenGLShaderProgram shader_;
    GLuint vao_ = 0, vbo_ = 0, ebo_ = 0;

    QMatrix4x4 proj_, view_, modelMat_;

    QVector3D eye_;
    QVector3D target_{0, 0, 0};
    float camDist_ = 3.5f;

    float yaw_ = 45.0f;
    float pitch_ = 45.0f;

    QPoint lastMousePos_;
};


#endif //GLWIDGET_H
