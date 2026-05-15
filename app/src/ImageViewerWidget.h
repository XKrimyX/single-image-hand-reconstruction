#pragma once

#include <QGraphicsPixmapItem>
#include <QGraphicsView>
#include <QImage>
#include <QWidget>

class QLabel;

class ImageGraphicsView : public QGraphicsView
{
    Q_OBJECT

public:
    explicit ImageGraphicsView(QGraphicsScene *scene, QWidget *parent = nullptr);

signals:
    void zoomRequested(double factor);

protected:
    void wheelEvent(QWheelEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
};

class ImageViewerWidget : public QWidget
{
    Q_OBJECT

public:
    explicit ImageViewerWidget(QWidget *parent = nullptr);

    bool loadImage(const QString &filePath);
    bool saveCurrentImage(const QString &filePath) const;
    bool hasImage() const;
    QString currentImagePath() const;

public slots:
    void zoomIn();
    void zoomOut();
    void fitToWindow();
    void rotateLeft();
    void rotateRight();
    void mirrorHorizontal();
    void resetView();

signals:
    void imageLoaded(const QString &filePath, const QSize &size);

private:
    void updatePixmap();

    ImageGraphicsView *m_view = nullptr;
    QGraphicsScene *m_scene = nullptr;
    QGraphicsPixmapItem *m_pixmapItem = nullptr;
    QLabel *m_hintLabel = nullptr;

    QImage m_image;
    QString m_currentImagePath;
    double m_scale = 1.0;
    int m_rotation = 0;
    bool m_mirrored = false;
};
