#pragma once

#include <QWidget>

class QLabel;
class QGridLayout;
class QTextEdit;

class ReconstructionPanel : public QWidget
{
    Q_OBJECT

public:
    explicit ReconstructionPanel(QWidget *parent = nullptr);

    void setCurrentImage(const QString &path);
    void setImageStatus(const QString &status);
    void setCameraStatus(const QString &status);
    void setPreprocessStatus(const QString &status);
    void setModelStatus(const QString &status);
    void setMeshStats(int vertices, int faces);
    void setInferenceTime(const QString &timeText);
    void setOutputPath(const QString &path);
    void appendLog(const QString &message);

private:
    QLabel *makeValueLabel(const QString &text);
    QWidget *makeInfoCell(const QString &name, QLabel *valueLabel);

    QLabel *m_currentImage = nullptr;
    QLabel *m_imageStatus = nullptr;
    QLabel *m_cameraStatus = nullptr;
    QLabel *m_preprocessStatus = nullptr;
    QLabel *m_modelStatus = nullptr;
    QLabel *m_meshStats = nullptr;
    QLabel *m_inferenceTime = nullptr;
    QLabel *m_outputPath = nullptr;
    QTextEdit *m_log = nullptr;
};
