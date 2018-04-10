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
    connect(ui->FilePickerButton, &QToolButton::clicked, this, &SettingsWindow::ShowFilePicker);
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

QString SettingsWindow::GetBroadcastMode()
{
    return ui->BroadcastModeComboBox->currentText();
}

QString SettingsWindow::GetFileName()
{
    return ui->FileNameLineEdit->text();
}

void SettingsWindow::ToggleClientServerUi(QString ignoreThis)
{
    if (GetHostMode() == "Client")
    {
        ui->IpLineEdit->setEnabled(true);
        ui->BroadcastModeComboBox->setEnabled(false);
        ui->FileNameLineEdit->setEnabled(false);
        ui->FilePickerButton->setEnabled(false);
    }
    if (GetHostMode() == "Server")
    {
        ui->IpLineEdit->setEnabled(false);
        if (GetTransferMode() == "streaming")
        {
            ui->BroadcastModeComboBox->setEnabled(true);
            ui->FileNameLineEdit->setEnabled(true);
            ui->FilePickerButton->setEnabled(true);
        }
        else
        {
            ui->BroadcastModeComboBox->setEnabled(false);
            ui->FileNameLineEdit->setEnabled(false);
            ui->FilePickerButton->setEnabled(false);
        }
    }
}

void SettingsWindow::ShowFilePicker()
{
    QString fileName = QFileDialog::getOpenFileName(this,
        tr("Choose a file to stream"), "", tr("(*.wav)"));
    ui->FileNameLineEdit->setText(fileName);
}
