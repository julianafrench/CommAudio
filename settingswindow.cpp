#include "settingswindow.h"
#include "ui_settingswindow.h"

SettingsWindow::SettingsWindow(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::SettingsWindow)
{
    ui->setupUi(this);
    ToggleClientServerUi();
    connect(ui->HostModeComboBox, &QComboBox::currentTextChanged, this, &SettingsWindow::ToggleClientServerUi);
    connect(ui->TransferModeComboBox, &QComboBox::currentTextChanged, this, &SettingsWindow::ToggleClientServerUi);
}

SettingsWindow::~SettingsWindow()
{
    delete ui;
}

QString SettingsWindow::GetHostMode()
{
    return ui->HostModeComboBox->currentText();
}

QString SettingsWindow::GetIpAddress()
{
    return ui->IpLineEdit->text().trimmed();
}

QString SettingsWindow::GetTransferMode()
{
    return ui->TransferModeComboBox->currentText();
}

QString SettingsWindow::GetFileName()
{
    return selectedFileName;
}

void SettingsWindow::ToggleClientServerUi(QString ignoreThis)
{
    if (GetHostMode() == "Client")
        ui->IpLineEdit->setEnabled(true);
    if (GetHostMode() == "Server")
        ui->IpLineEdit->setEnabled(false);
}

void SettingsWindow::SetFileName(QString fileName)
{
    selectedFileName = fileName;
}
