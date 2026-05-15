#include "ImageViewerWidget.h"

#include <QFileInfo>
#include <QGraphicsScene>
#include <QLabel>
#include <QMouseEvent>
#include <QScrollBar>
#include <QVBoxLayout>
#include <QWheelEvent>

ImageGraphicsView::ImageGraphicsView(QGraphicsScene *scene, QWidget *parent)
    : QGraphicsView(scene, parent)
{
    setDragMode(QGraphicsView::ScrollHandDrag);
    setTransformationAnchor(QGraphicsView::AnchorUnderMouse);
    setResizeAnchor(QGraphicsView::AnchorViewCenter);
    setCursor(Qt::OpenHandCursor);
}

void ImageGraphicsView::wheelEvent(QWheelEvent *event)
{
    // 鼠标滚轮缩放只通知外层控件，由 ImageViewerWidget 统一维护缩放倍数。
    emit zoomRequested(event->angleDelta().y() > 0 ? 1.15 : 1.0 / 1.15);
    event->accept();
}

void ImageGraphicsView::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        setCursor(Qt::ClosedHandCursor);
    }
    QGraphicsView::mousePressEvent(event);
}

void ImageGraphicsView::mouseReleaseEvent(QMouseEvent *event)
{
    setCursor(Qt::OpenHandCursor);
    QGraphicsView::mouseReleaseEvent(event);
}

ImageViewerWidget::ImageViewerWidget(QWidget *parent)
    : QWidget(parent)
{
    m_scene = new QGraphicsScene(this);
    m_view = new ImageGraphicsView(m_scene, this);
    m_view->setBackgroundBrush(QColor("#f7f9fc"));
    m_view->setRenderHint(QPainter::SmoothPixmapTransform, true);
    m_view->setFrameShape(QFrame::NoFrame);
    connect(m_view, &ImageGraphicsView::zoomRequested, this, [this](double factor) {
        if (!hasImage()) {
            return;
        }
        m_scale *= factor;
        updatePixmap();
    });

    m_pixmapItem = m_scene->addPixmap(QPixmap());
    m_pixmapItem->setTransformationMode(Qt::SmoothTransformation);

    m_hintLabel = new QLabel("打开 JPG、PNG 或 BMP 图片后，这里显示二维图像", this);
    m_hintLabel->setAlignment(Qt::AlignCenter);
    m_hintLabel->setStyleSheet("color: #667085; padding: 24px;");

    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(m_view);
    layout->addWidget(m_hintLabel);
}

bool ImageViewerWidget::loadImage(const QString &filePath)
{
    QImage image(filePath);
    if (image.isNull()) {
        return false;
    }

    m_image = image;
    m_currentImagePath = filePath;
    m_scale = 1.0;
    m_rotation = 0;
    m_mirrored = false;
    updatePixmap();
    fitToWindow();
    emit imageLoaded(filePath, image.size());
    return true;
}

bool ImageViewerWidget::saveCurrentImage(const QString &filePath) const
{
    if (m_image.isNull()) {
        return false;
    }

    QTransform transform;
    if (m_mirrored) {
        transform.scale(-1, 1);
    }
    transform.rotate(m_rotation);
    return m_image.transformed(transform, Qt::SmoothTransformation).save(filePath);
}

bool ImageViewerWidget::hasImage() const
{
    return !m_image.isNull();
}

QString ImageViewerWidget::currentImagePath() const
{
    return m_currentImagePath;
}

void ImageViewerWidget::zoomIn()
{
    if (!hasImage()) {
        return;
    }
    m_scale *= 1.2;
    updatePixmap();
}

void ImageViewerWidget::zoomOut()
{
    if (!hasImage()) {
        return;
    }
    m_scale /= 1.2;
    updatePixmap();
}

void ImageViewerWidget::fitToWindow()
{
    if (!hasImage()) {
        return;
    }
    m_view->fitInView(m_pixmapItem, Qt::KeepAspectRatio);
}

void ImageViewerWidget::rotateLeft()
{
    if (!hasImage()) {
        return;
    }
    m_rotation = (m_rotation + 270) % 360;
    updatePixmap();
    fitToWindow();
}

void ImageViewerWidget::rotateRight()
{
    if (!hasImage()) {
        return;
    }
    m_rotation = (m_rotation + 90) % 360;
    updatePixmap();
    fitToWindow();
}

void ImageViewerWidget::mirrorHorizontal()
{
    if (!hasImage()) {
        return;
    }
    m_mirrored = !m_mirrored;
    updatePixmap();
    fitToWindow();
}

void ImageViewerWidget::resetView()
{
    if (!hasImage()) {
        return;
    }
    m_scale = 1.0;
    m_rotation = 0;
    m_mirrored = false;
    updatePixmap();
    fitToWindow();
}

void ImageViewerWidget::updatePixmap()
{
    QTransform transform;
    if (m_mirrored) {
        transform.scale(-1, 1);
    }
    transform.rotate(m_rotation);

    QImage transformed = m_image.transformed(transform, Qt::SmoothTransformation);
    QPixmap pixmap = QPixmap::fromImage(transformed);
    if (m_scale != 1.0) {
        pixmap = pixmap.scaled(pixmap.size() * m_scale, Qt::KeepAspectRatio, Qt::SmoothTransformation);
    }

    m_pixmapItem->setPixmap(pixmap);
    m_scene->setSceneRect(m_pixmapItem->boundingRect());
    m_hintLabel->setVisible(!hasImage());
}
