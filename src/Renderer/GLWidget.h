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

    void setShowGrid(bool on);

    void setLightYaw(int deg);       // 0-360
    void setLightPitch(int deg);     // -89~89
    void setLightRadius(double r);   // 거리
    void setKd(int v);               // 0-100 -> 0-1
    void setKs(int v);
    void setShininess(int v);

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

    void loadCube();

    void loadShaders(QOpenGLShaderProgram& program ,const QString &vert, const QString &frag);

    void uploadVertexBuffer();

    void setModelMat();

    void updateCamera();

    void drawGrid();

    void drawModel();

    void drawLight();

    void updateLight();

    QTimer timer_;

    //Shader
    QOpenGLShaderProgram phongProg_;
    QOpenGLShaderProgram gridProg_;

    // Model
    ModelLoader model_;
    GLuint vaoModel_ = 0, vboModel_ = 0, eboModel_ = 0;

    // Grid Cube
    ModelLoader cube_;
    GLuint vaoGrid_ = 0, vboGrid_ = 0, eboGrid_ = 0;
    bool showGrid_ = true;

    QMatrix4x4 proj_, view_, modelMat_;

    QVector3D eye_;
    QVector3D target_{0, 0, 0};
    float camDist_ = 3.5f;

    float yaw_ = 45.0f;
    float pitch_ = -45.0f;

    QPoint lastMousePos_;

    //Lighting parameters
    float lightYaw_    = 45.0f;   // 도(deg)  0 = +X,  90 = +Z
    float lightPitch_  = 30.0f;   // -89 ~ +89 (위/아래)
    float lightRadius_ = 3.0f;    // 타깃까지 거리
    QVector3D lightPos_;

    float     kd_ = 1.0f;             // Diffuse  (0-1)
    float     ks_ = 0.4f;             // Specular (0-1)
    float     shininess_ = 32.0f;


};


#endif //GLWIDGET_H
