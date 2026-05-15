#pragma once

#include <QOpenGLWidget>
#include <QPoint>
#include <QVector>
#include <QVector3D>

struct MeshFace
{
    int a = 0;
    int b = 0;
    int c = 0;
};

class ModelViewerWidget : public QOpenGLWidget
{
    Q_OBJECT

public:
    explicit ModelViewerWidget(QWidget *parent = nullptr);

    void setMeshInfo(const QString &meshPath, int vertices, int faces);
    void clearMesh();
    QString meshPath() const;
    QImage renderToImage();

public slots:
    void resetView();
    void fitModel();
    void setWireframeMode(bool enabled);

protected:
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void wheelEvent(QWheelEvent *event) override;
    void paintEvent(QPaintEvent *event) override;

private:
    bool loadMesh(const QString &meshPath);
    bool loadObj(const QString &meshPath);
    bool loadAsciiStl(const QString &meshPath);
    bool loadBinaryStl(const QString &meshPath);
    void computeBounds();
    QVector3D transformVertex(const QVector3D &vertex) const;
    QPointF projectVertex(const QVector3D &vertex, const QRectF &area) const;
    void drawPlaceholder(QPainter &painter, const QRect &area);
    void drawMesh(QPainter &painter, const QRectF &area);
    void drawScene(QPainter &painter);

    QString m_meshPath;
    int m_vertices = 0;
    int m_faces = 0;
    bool m_wireframe = false;
    bool m_meshLoaded = false;
    QString m_errorMessage;

    QVector<QVector3D> m_verticesData;
    QVector<MeshFace> m_facesData;
    QVector3D m_center;
    float m_radius = 1.0f;
    float m_rotationX = -20.0f;
    float m_rotationY = 25.0f;
    float m_zoom = 1.0f;
    QPointF m_pan;
    QPoint m_lastMousePos;
};
