#include "SettingsDialog.h"

#include <QCheckBox>
#include <QDialogButtonBox>
#include <QFileDialog>
#include <QGridLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QVBoxLayout>

SettingsDialog::SettingsDialog(const AppConfig &config, QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle("设置");
    setMinimumWidth(720);
    setStyleSheet(
        "QDialog { background: #f5f8fb; color: #243447; }"
        "QLabel { color: #243447; font-weight: 600; }"
        "QLineEdit { padding: 7px; border: 1px solid #b8c7d9; border-radius: 4px; background: #ffffff; color: #243447; }"
        "QPushButton { padding: 7px 12px; border: 1px solid #b8c7d9; border-radius: 5px; background: #ffffff; color: #243447; }"
        "QPushButton:hover { background: #eef6ff; }"
        "QCheckBox { color: #243447; spacing: 8px; }");

    auto *grid = new QGridLayout;
    grid->setHorizontalSpacing(10);
    grid->setVerticalSpacing(10);

    m_pythonEdit = addPathRow(grid, 0, "Python 解释器", config.pythonInterpreter, false);
    m_cameraEdit = addPathRow(grid, 1, "相机参数", config.cameraParamsPath, false);
    m_undistortEdit = addPathRow(grid, 2, "去畸变脚本", config.undistortScriptPath, false);
    m_reconstructEdit = addPathRow(grid, 3, "重建脚本", config.reconstructScriptPath, false);
    m_checkpointEdit = addPathRow(grid, 4, "simpleHand checkpoint", config.simplehandCheckpoint, false);
    m_outputDirEdit = addPathRow(grid, 5, "输出目录", config.outputDir, true);

    auto *deviceLabel = new QLabel("推理设备", this);
    m_deviceEdit = new QLineEdit(config.reconstructDevice, this);
    m_deviceEdit->setPlaceholderText("auto / cpu / cuda");
    grid->addWidget(deviceLabel, 6, 0);
    grid->addWidget(m_deviceEdit, 6, 1);

    m_autoLoadCamera = new QCheckBox("启动时自动加载相机参数", this);
    m_autoLoadCamera->setChecked(config.autoLoadCameraParams);
    m_autoUndistortBeforeReconstruct = new QCheckBox("重建前自动使用去畸变结果", this);
    m_autoUndistortBeforeReconstruct->setChecked(config.autoUndistortBeforeReconstruct);

    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    connect(buttons, &QDialogButtonBox::accepted, this, &SettingsDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, this, &SettingsDialog::reject);

    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(18, 16, 18, 16);
    layout->setSpacing(12);
    layout->addLayout(grid);
    layout->addWidget(m_autoLoadCamera);
    layout->addWidget(m_autoUndistortBeforeReconstruct);
    layout->addWidget(buttons);
}

AppConfig SettingsDialog::config() const
{
    AppConfig config;
    config.pythonInterpreter = m_pythonEdit->text().trimmed();
    config.cameraParamsPath = m_cameraEdit->text().trimmed();
    config.undistortScriptPath = m_undistortEdit->text().trimmed();
    config.reconstructScriptPath = m_reconstructEdit->text().trimmed();
    config.simplehandCheckpoint = m_checkpointEdit->text().trimmed();
    config.reconstructDevice = m_deviceEdit->text().trimmed().isEmpty() ? "auto" : m_deviceEdit->text().trimmed();
    config.outputDir = m_outputDirEdit->text().trimmed();
    config.autoLoadCameraParams = m_autoLoadCamera->isChecked();
    config.autoUndistortBeforeReconstruct = m_autoUndistortBeforeReconstruct->isChecked();
    return config;
}

QLineEdit *SettingsDialog::addPathRow(QGridLayout *layout, int row, const QString &labelText, const QString &value, bool directory)
{
    auto *label = new QLabel(labelText, this);
    auto *edit = new QLineEdit(value, this);
    auto *button = new QPushButton("选择", this);

    connect(button, &QPushButton::clicked, this, [this, edit, directory]() {
        choosePath(edit, directory);
    });

    layout->addWidget(label, row, 0);
    layout->addWidget(edit, row, 1);
    layout->addWidget(button, row, 2);
    return edit;
}

void SettingsDialog::choosePath(QLineEdit *edit, bool directory)
{
    const QString start = edit->text().isEmpty() ? AppConfig::projectRoot() : edit->text();
    const QString path = directory
        ? QFileDialog::getExistingDirectory(this, "选择目录", start)
        : QFileDialog::getOpenFileName(this, "选择文件", start);
    if (!path.isEmpty()) {
        edit->setText(path);
    }
}
