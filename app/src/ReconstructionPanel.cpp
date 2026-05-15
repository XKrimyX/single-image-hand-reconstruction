#include "ReconstructionPanel.h"

#include <QFileInfo>
#include <QGridLayout>
#include <QLabel>
#include <QTextEdit>
#include <QHBoxLayout>
#include <QVBoxLayout>

ReconstructionPanel::ReconstructionPanel(QWidget *parent)
    : QWidget(parent)
{
    setObjectName("ReconstructionPanel");

    auto *title = new QLabel("重建信息", this);
    title->setObjectName("PanelTitle");

    auto *grid = new QGridLayout;
    grid->setHorizontalSpacing(14);
    grid->setVerticalSpacing(10);

    m_currentImage = makeValueLabel("未打开");
    m_imageStatus = makeValueLabel("未加载");
    m_cameraStatus = makeValueLabel("未加载");
    m_preprocessStatus = makeValueLabel("未处理");
    m_modelStatus = makeValueLabel("未重建");
    m_meshStats = makeValueLabel("-");
    m_inferenceTime = makeValueLabel("-");
    m_outputPath = makeValueLabel("-");

    grid->addWidget(makeInfoCell("当前图片", m_currentImage), 0, 0);
    grid->addWidget(makeInfoCell("图片状态", m_imageStatus), 0, 1);
    grid->addWidget(makeInfoCell("相机参数", m_cameraStatus), 0, 2);
    grid->addWidget(makeInfoCell("预处理状态", m_preprocessStatus), 0, 3);
    grid->addWidget(makeInfoCell("模型状态", m_modelStatus), 1, 0);
    grid->addWidget(makeInfoCell("顶点 / 面片", m_meshStats), 1, 1);
    grid->addWidget(makeInfoCell("推理耗时", m_inferenceTime), 1, 2);
    grid->addWidget(makeInfoCell("输出路径", m_outputPath), 1, 3);
    for (int column = 0; column < 4; ++column) {
        grid->setColumnStretch(column, 1);
    }

    m_log = new QTextEdit(this);
    m_log->setObjectName("RunLog");
    m_log->setReadOnly(true);
    m_log->setMaximumHeight(58);
    m_log->setPlaceholderText("运行日志");

    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(12, 8, 12, 10);
    layout->setSpacing(8);
    layout->addWidget(title);
    layout->addLayout(grid);
    layout->addWidget(m_log);
}

void ReconstructionPanel::setCurrentImage(const QString &path)
{
    m_currentImage->setText(path.isEmpty() ? "未打开" : QFileInfo(path).fileName());
    m_currentImage->setToolTip(path);
}

void ReconstructionPanel::setImageStatus(const QString &status)
{
    m_imageStatus->setText(status);
}

void ReconstructionPanel::setCameraStatus(const QString &status)
{
    m_cameraStatus->setText(status);
}

void ReconstructionPanel::setPreprocessStatus(const QString &status)
{
    m_preprocessStatus->setText(status);
}

void ReconstructionPanel::setModelStatus(const QString &status)
{
    m_modelStatus->setText(status);
}

void ReconstructionPanel::setMeshStats(int vertices, int faces)
{
    m_meshStats->setText(QString("%1 / %2").arg(vertices).arg(faces));
}

void ReconstructionPanel::setInferenceTime(const QString &timeText)
{
    m_inferenceTime->setText(timeText);
}

void ReconstructionPanel::setOutputPath(const QString &path)
{
    m_outputPath->setText(path.isEmpty() ? "-" : path);
    m_outputPath->setToolTip(path);
}

void ReconstructionPanel::appendLog(const QString &message)
{
    m_log->append(message);
}

QLabel *ReconstructionPanel::makeValueLabel(const QString &text)
{
    auto *label = new QLabel(text, this);
    label->setObjectName("InfoValue");
    label->setTextInteractionFlags(Qt::TextSelectableByMouse);
    label->setWordWrap(true);
    return label;
}

QWidget *ReconstructionPanel::makeInfoCell(const QString &name, QLabel *valueLabel)
{
    auto *cell = new QWidget(this);
    cell->setObjectName("InfoCell");

    auto *nameLabel = new QLabel(name, cell);
    nameLabel->setObjectName("InfoName");

    auto *layout = new QHBoxLayout(cell);
    layout->setContentsMargins(10, 6, 10, 6);
    layout->setSpacing(8);
    layout->addWidget(nameLabel);
    layout->addWidget(valueLabel, 1);

    return cell;
}
