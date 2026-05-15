#pragma once

#include "AppConfig.h"

#include <QDialog>

class QCheckBox;
class QGridLayout;
class QLineEdit;

class SettingsDialog : public QDialog
{
    Q_OBJECT

public:
    explicit SettingsDialog(const AppConfig &config, QWidget *parent = nullptr);

    AppConfig config() const;

private:
    QLineEdit *addPathRow(QGridLayout *layout, int row, const QString &labelText, const QString &value, bool directory);
    void choosePath(QLineEdit *edit, bool directory);

    QLineEdit *m_pythonEdit = nullptr;
    QLineEdit *m_cameraEdit = nullptr;
    QLineEdit *m_undistortEdit = nullptr;
    QLineEdit *m_reconstructEdit = nullptr;
    QLineEdit *m_checkpointEdit = nullptr;
    QLineEdit *m_deviceEdit = nullptr;
    QLineEdit *m_outputDirEdit = nullptr;
    QCheckBox *m_autoLoadCamera = nullptr;
    QCheckBox *m_autoUndistortBeforeReconstruct = nullptr;
};
