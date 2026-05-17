#include "ModelViewerWidget.h"

#include <QDataStream>
#include <QFile>
#include <QFileInfo>
#include <QMouseEvent>
#include <QPainter>
#include <QTextStream>
#include <QWheelEvent>
#include <algorithm>
#include <cmath>

ModelViewerWidget::ModelViewerWidget(QWidget *parent)
    : QOpenGLWidget(parent)
{
    setMinimumSize(360, 320);
    setMouseTracking(true);
}

void ModelViewerWidget::setMeshInfo(const QString &meshPath, int vertices, int faces)
{
    m_meshPath = meshPath;
    m_vertices = vertices;
    m_faces = faces;
    m_meshLoaded = loadMesh(meshPath);
    if (m_meshLoaded) {
        m_vertices = m_verticesData.size();
        m_faces = m_facesData.size();
        fitModel();
    }
    update();
}

void ModelViewerWidget::clearMesh()
{
    m_meshPath.clear();
    m_vertices = 0;
    m_faces = 0;
    m_meshLoaded = false;
    m_errorMessage.clear();
    m_verticesData.clear();
    m_facesData.clear();
    update();
}

QString ModelViewerWidget::meshPath() const
{
    return m_meshPath;
}

QImage ModelViewerWidget::renderToImage()
{
    QImage image(size() * devicePixelRatioF(), QImage::Format_ARGB32_Premultiplied);
    image.setDevicePixelRatio(devicePixelRatioF());
    image.fill(QColor("#eef3f8"));
    QPainter painter(&image);
    painter.setRenderHint(QPainter::Antialiasing, true);
    drawScene(painter);
    return image;
}

void ModelViewerWidget::resetView()
{
    m_rotationX = -20.0f;
    m_rotationY = 25.0f;
    m_zoom = 1.0f;
    m_pan = QPointF(0, 0);
    update();
}

void ModelViewerWidget::fitModel()
{
    m_zoom = 1.0f;
    m_pan = QPointF(0, 0);
    update();
}

void ModelViewerWidget::setWireframeMode(bool enabled)
{
    m_wireframe = enabled;
    update();
}

void ModelViewerWidget::mousePressEvent(QMouseEvent *event)
{
    m_lastMousePos = event->pos();
}

void ModelViewerWidget::mouseMoveEvent(QMouseEvent *event)
{
    const QPoint delta = event->pos() - m_lastMousePos;
    m_lastMousePos = event->pos();

    if (event->buttons() & Qt::LeftButton) {
        // 左键拖动旋转模型，角度变化和鼠标移动距离成正比。
        m_rotationY += delta.x() * 0.6f;
        m_rotationX += delta.y() * 0.6f;
        update();
    } else if ((event->buttons() & Qt::RightButton) || (event->buttons() & Qt::MiddleButton)) {
        // 右键或中键拖动平移模型。
        m_pan += QPointF(delta.x(), delta.y());
        update();
    }
}

void ModelViewerWidget::wheelEvent(QWheelEvent *event)
{
    const float factor = event->angleDelta().y() > 0 ? 1.12f : 1.0f / 1.12f;
    m_zoom = std::clamp(m_zoom * factor, 0.15f, 12.0f);
    update();
}

void ModelViewerWidget::paintEvent(QPaintEvent *event)
{
    QOpenGLWidget::paintEvent(event);

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);
    drawScene(painter);
}

void ModelViewerWidget::drawScene(QPainter &painter)
{
    painter.fillRect(rect(), QColor("#eef3f8"));

    const QRect area = rect().adjusted(24, 24, -24, -24);
    painter.setPen(QPen(QColor("#c7d4e5"), 1));
    painter.setBrush(QColor("#f8fbff"));
    painter.drawRoundedRect(area, 6, 6);

    if (!m_meshLoaded) {
        drawPlaceholder(painter, area);
        return;
    }

    drawMesh(painter, area);
}

bool ModelViewerWidget::loadMesh(const QString &meshPath)
{
    m_errorMessage.clear();
    m_verticesData.clear();
    m_facesData.clear();

    const QFileInfo info(meshPath);
    if (!info.exists()) {
        m_errorMessage = "模型文件不存在：" + meshPath;
        return false;
    }

    const QString suffix = info.suffix().toLower();
    bool ok = false;
    if (suffix == "obj") {
        ok = loadObj(meshPath);
    } else if (suffix == "stl") {
        // STL 有 ASCII 和二进制两种，先按文本读，失败后再按二进制读。
        ok = loadAsciiStl(meshPath);
        if (!ok) {
            ok = loadBinaryStl(meshPath);
        }
    } else {
        m_errorMessage = "暂不支持的模型格式：" + suffix;
        return false;
    }

    if (!ok || m_verticesData.isEmpty() || m_facesData.isEmpty()) {
        if (m_errorMessage.isEmpty()) {
            m_errorMessage = "模型文件中没有读到有效三角网格。";
        }
        return false;
    }

    computeBounds();
    return true;
}

bool ModelViewerWidget::loadObj(const QString &meshPath)
{
    QFile file(meshPath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        m_errorMessage = "无法打开 OBJ：" + meshPath;
        return false;
    }

    QTextStream stream(&file);
    while (!stream.atEnd()) {
        const QString line = stream.readLine().trimmed();
        if (line.startsWith("v ")) {
            const QStringList parts = line.split(' ', Qt::SkipEmptyParts);
            if (parts.size() >= 4) {
                m_verticesData.append(QVector3D(parts[1].toFloat(), parts[2].toFloat(), parts[3].toFloat()));
            }
        } else if (line.startsWith("f ")) {
            const QStringList parts = line.split(' ', Qt::SkipEmptyParts);
            if (parts.size() >= 4) {
                QVector<int> indices;
                for (int i = 1; i < parts.size(); ++i) {
                    const QString token = parts[i].split('/').first();
                    int index = token.toInt();
                    if (index < 0) {
                        index = m_verticesData.size() + index;
                    } else {
                        index -= 1;
                    }
                    indices.append(index);
                }
                // OBJ 面可能是四边形或更多边形，这里用扇形法拆成三角形。
                for (int i = 1; i + 1 < indices.size(); ++i) {
                    m_facesData.append({indices[0], indices[i], indices[i + 1]});
                }
            }
        }
    }

    return true;
}

bool ModelViewerWidget::loadAsciiStl(const QString &meshPath)
{
    QFile file(meshPath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return false;
    }

    QTextStream stream(&file);
    QVector<int> currentFace;
    while (!stream.atEnd()) {
        const QString line = stream.readLine().trimmed();
        if (!line.startsWith("vertex ")) {
            continue;
        }
        const QStringList parts = line.split(' ', Qt::SkipEmptyParts);
        if (parts.size() < 4) {
            continue;
        }
        m_verticesData.append(QVector3D(parts[1].toFloat(), parts[2].toFloat(), parts[3].toFloat()));
        currentFace.append(m_verticesData.size() - 1);
        if (currentFace.size() == 3) {
            m_facesData.append({currentFace[0], currentFace[1], currentFace[2]});
            currentFace.clear();
        }
    }

    return !m_verticesData.isEmpty() && !m_facesData.isEmpty();
}

bool ModelViewerWidget::loadBinaryStl(const QString &meshPath)
{
    QFile file(meshPath);
    if (!file.open(QIODevice::ReadOnly)) {
        return false;
    }

    if (file.size() < 84) {
        return false;
    }

    file.seek(80);
    QDataStream stream(&file);
    stream.setByteOrder(QDataStream::LittleEndian);

    quint32 triangleCount = 0;
    stream >> triangleCount;
    m_verticesData.reserve(triangleCount * 3);
    m_facesData.reserve(triangleCount);

    for (quint32 i = 0; i < triangleCount && !stream.atEnd(); ++i) {
        float ignoredNormal[3];
        stream >> ignoredNormal[0] >> ignoredNormal[1] >> ignoredNormal[2];

        const int baseIndex = m_verticesData.size();
        for (int vertex = 0; vertex < 3; ++vertex) {
            float x = 0;
            float y = 0;
            float z = 0;
            stream >> x >> y >> z;
            m_verticesData.append(QVector3D(x, y, z));
        }

        quint16 attribute = 0;
        stream >> attribute;
        m_facesData.append({baseIndex, baseIndex + 1, baseIndex + 2});
    }

    return !m_verticesData.isEmpty() && !m_facesData.isEmpty();
}

void ModelViewerWidget::computeBounds()
{
    QVector3D minPoint = m_verticesData.first();
    QVector3D maxPoint = m_verticesData.first();
    for (const QVector3D &vertex : m_verticesData) {
        minPoint.setX(std::min(minPoint.x(), vertex.x()));
        minPoint.setY(std::min(minPoint.y(), vertex.y()));
        minPoint.setZ(std::min(minPoint.z(), vertex.z()));
        maxPoint.setX(std::max(maxPoint.x(), vertex.x()));
        maxPoint.setY(std::max(maxPoint.y(), vertex.y()));
        maxPoint.setZ(std::max(maxPoint.z(), vertex.z()));
    }

    m_center = (minPoint + maxPoint) * 0.5f;
    m_radius = std::max(0.0001f, (maxPoint - minPoint).length() * 0.5f);
}

QVector3D ModelViewerWidget::transformVertex(const QVector3D &vertex) const
{
    QVector3D p = vertex - m_center;

    const float rx = qDegreesToRadians(m_rotationX);
    const float ry = qDegreesToRadians(m_rotationY);
    const float cosX = std::cos(rx);
    const float sinX = std::sin(rx);
    const float cosY = std::cos(ry);
    const float sinY = std::sin(ry);

    const float y = p.y() * cosX - p.z() * sinX;
    const float z = p.y() * sinX + p.z() * cosX;
    p.setY(y);
    p.setZ(z);

    const float x = p.x() * cosY + p.z() * sinY;
    const float z2 = -p.x() * sinY + p.z() * cosY;
    p.setX(x);
    p.setZ(z2);

    return p;
}

QPointF ModelViewerWidget::projectVertex(const QVector3D &vertex, const QRectF &area) const
{
    const float scale = 0.42f * std::min(area.width(), area.height()) * m_zoom / m_radius;
    const QPointF center = area.center() + m_pan;
    return QPointF(center.x() + vertex.x() * scale, center.y() - vertex.y() * scale);
}

void ModelViewerWidget::drawPlaceholder(QPainter &painter, const QRect &area)
{
    painter.setPen(QPen(QColor("#9aa9bb"), 1));
    const QPoint center = area.center();
    painter.drawLine(area.left() + 40, center.y(), area.right() - 40, center.y());
    painter.drawLine(center.x(), area.top() + 40, center.x(), area.bottom() - 40);

    painter.setPen(QColor("#344054"));
    QFont titleFont = painter.font();
    titleFont.setPointSize(15);
    titleFont.setBold(true);
    painter.setFont(titleFont);
    painter.drawText(area.adjusted(0, 64, 0, 0), Qt::AlignHCenter, "三维模型预览");

    QFont bodyFont = painter.font();
    bodyFont.setPointSize(11);
    bodyFont.setBold(false);
    painter.setFont(bodyFont);
    painter.setPen(QColor("#667085"));
    const QString message = m_errorMessage.isEmpty()
        ? "重建完成后这里会加载 OBJ/STL 网格。\n左键旋转，右键/中键平移，滚轮缩放。"
        : m_errorMessage;
    painter.drawText(area.adjusted(24, 112, -24, -24), Qt::AlignHCenter | Qt::AlignTop | Qt::TextWordWrap, message);
}

void ModelViewerWidget::drawMesh(QPainter &painter, const QRectF &area)
{
    struct FaceDrawItem
    {
        QPolygonF polygon;
        float depth = 0.0f;
        float light = 0.0f;
    };

    QVector<FaceDrawItem> items;
    items.reserve(m_facesData.size());

    // 这个查看器是轻量预览：自己做旋转、投影，再按深度排序画三角面。
    for (const MeshFace &face : m_facesData) {
        if (face.a < 0 || face.b < 0 || face.c < 0 ||
            face.a >= m_verticesData.size() || face.b >= m_verticesData.size() || face.c >= m_verticesData.size()) {
            continue;
        }

        const QVector3D a = transformVertex(m_verticesData[face.a]);
        const QVector3D b = transformVertex(m_verticesData[face.b]);
        const QVector3D c = transformVertex(m_verticesData[face.c]);
        const QVector3D normal = QVector3D::crossProduct(b - a, c - a).normalized();
        const float light = std::clamp(QVector3D::dotProduct(normal, QVector3D(0.2f, -0.4f, 1.0f).normalized()), 0.15f, 1.0f);

        FaceDrawItem item;
        item.depth = (a.z() + b.z() + c.z()) / 3.0f;
        item.light = light;
        item.polygon << projectVertex(a, area) << projectVertex(b, area) << projectVertex(c, area);
        items.append(item);
    }

    std::sort(items.begin(), items.end(), [](const FaceDrawItem &left, const FaceDrawItem &right) {
        return left.depth < right.depth;
    });

    for (const FaceDrawItem &item : items) {
        const int blue = static_cast<int>(150 + item.light * 80);
        const QColor fillColor(66, 145, blue, m_wireframe ? 35 : 210);
        painter.setBrush(m_wireframe ? Qt::NoBrush : QBrush(fillColor));
        painter.setPen(QPen(QColor(39, 83, 128, m_wireframe ? 220 : 95), m_wireframe ? 1.0 : 0.6));
        painter.drawPolygon(item.polygon);
    }

    painter.setPen(QColor("#344054"));
    painter.drawText(
        area.adjusted(12, 12, -12, -12),
        Qt::AlignLeft | Qt::AlignTop,
        QString("%1\n顶点：%2  面片：%3  模式：%4")
            .arg(QFileInfo(m_meshPath).fileName())
            .arg(m_vertices)
            .arg(m_faces)
            .arg(m_wireframe ? "线框" : "实体"));
}
