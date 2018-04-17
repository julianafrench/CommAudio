#include "settingswindow.h"
#include "ui_settingswindow.h"
/******************************************************************************
 * SOURCE FILE:         settingswindow.cpp
 *
 * PROGRAM:             CommAudio
 *
 * FUNCTIONS:           public SettingsWindow(QWidget *parent)
 *                      public QString GetHostMode();
 *                      public QString GetTransferMode();
 *                      public QString GetIpAddress();
 *                      public QString GetFileName();
 *                      public void SetFileName(QString);
 *                      private void ToggleClientServerUi(QString ignoreThis = "");
 *
 * DATE:                Apr 9, 2018
 *
 * REVISIONS:           Apr 15, 2018 - reworked UI
 *
 * DESIGNER:            Alex Xia, Juliana French
 *
 * PROGRAMMER:          Alex Xia, Juliana French
 *
 * NOTES:
 * Settings window defines the behaviour of the settings dialog, allowing
 * the user to choose what kind of functionality they want from CommAudio.
 ******************************************************************************/

/******************************************************************************
 * FUNCTION:            SettingsWindow
 *
 * DATE:                Apr 9, 2018
 *
 * REVISIONS:
 *
 * DESIGNER:            Alex Xia
 *
 * PROGRAMMER:          Alex Xia
 *
 * INTERFACE:           public SettingsWindow(QWidget *parent)
 *
 * RETURNS:             instance of SettingsWindow
 *
 * NOTES:
 * Contructor for the Settings Window Dialog, connects all necessary
 * components so the user can select what type of host/transfer mode.
 ******************************************************************************/
SettingsWindow::SettingsWindow(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::SettingsWindow)
{
    ui->setupUi(this);
    ToggleClientServerUi();
    connect(ui->HostModeComboBox, &QComboBox::currentTextChanged, this, &SettingsWindow::ToggleClientServerUi);
    connect(ui->TransferModeComboBox, &QComboBox::currentTextChanged, this, &SettingsWindow::ToggleClientServerUi);
}

/******************************************************************************
 * FUNCTION:            ~SettingsWindow
 *
 * DATE:                Apr 9, 2018
 *
 * REVISIONS:
 *
 * DESIGNER:            Alex Xia
 *
 * PROGRAMMER:          Alex Xia
 *
 * INTERFACE:           ~SettingsWindow()
 *
 * RETURNS:             void
 *
 * NOTES:
 * Destructor for the SettingsWindow.
 ******************************************************************************/
SettingsWindow::~SettingsWindow()
{
    delete ui;
}

/******************************************************************************
 * FUNCTION:            GetHostMode
 *
 * DATE:                Apr 9, 2018
 *
 * REVISIONS:
 *
 * DESIGNER:            Alex Xia
 *
 * PROGRAMMER:          Alex Xia
 *
 * INTERFACE:           QString GetHostMode()
 *
 * RETURNS:             QString; name of host
 *
 * NOTES:
 * Returns a string with the type of host (client or server).
 ******************************************************************************/
QString SettingsWindow::GetHostMode()
{
    return ui->HostModeComboBox->currentText();
}

/******************************************************************************
 * FUNCTION:            GetIpAddress()
 *
 * DATE:                Apr 9, 2018
 *
 * REVISIONS:
 *
 * DESIGNER:            Alex Xia
 *
 * PROGRAMMER:          Alex Xia
 *
 * INTERFACE:           QString GetIpAddress()
 *
 * RETURNS:             QString; IP address
 *
 * NOTES:
 * Returns the IP address entered by the user.
 ******************************************************************************/
QString SettingsWindow::GetIpAddress()
{
    return ui->IpLineEdit->text().trimmed();
}

/******************************************************************************
 * FUNCTION:            GetTransferMode
 *
 * DATE:                Apr 9, 2018
 *
 * REVISIONS:
 *
 * DESIGNER:            Alex Xia
 *
 * PROGRAMMER:          Alex Xia
 *
 * INTERFACE:           QString GetTransferMode()
 *
 * RETURNS:             QString; type of audio transfer
 *
 * NOTES:
 * Returns a string of how CommAudio is transferring data (ie streaming,
 * file transfer).
 ******************************************************************************/
QString SettingsWindow::GetTransferMode()
{
    return ui->TransferModeComboBox->currentText();
}

/******************************************************************************
 * FUNCTION:            GetFileName
 *
 * DATE:                Apr 9, 2018
 *
 * REVISIONS:
 *
 * DESIGNER:            Alex Xia
 *
 * PROGRAMMER:          Alex Xia
 *
 * INTERFACE:           QString GetFileName()
 *
 * RETURNS:             QString; name of file
 *
 * NOTES:
 * Returns the name of the file selected.
 ******************************************************************************/
QString SettingsWindow::GetFileName()
{
    return selectedFileName;
}

/******************************************************************************
 * FUNCTION:            ToggleClientServerUi
 *
 * DATE:                Apr 9, 2018
 *
 * REVISIONS:
 *
 * DESIGNER:            Alex Xia
 *
 * PROGRAMMER:          Alex Xia
 *
 * INTERFACE:           void ToggleClientServerUi(QString ignoreThis)
 *                      ignoreThis - empty string
 *
 * RETURNS:             void
 *
 * NOTES:
 * Based on host mode, enables/disables IP address field in Settings Dialog UI.
 ******************************************************************************/
void SettingsWindow::ToggleClientServerUi(QString ignoreThis)
{
    if (GetHostMode() == "Client")
        ui->IpLineEdit->setEnabled(true);
    if (GetHostMode() == "Server")
        ui->IpLineEdit->setEnabled(false);
}

/******************************************************************************
 * FUNCTION:            SetFileName
 *
 * DATE:                Apr 9, 2018
 *
 * REVISIONS:           Apr 15 - made selectedFile a member variable
 *
 * DESIGNER:            Alex Xia
 *
 * PROGRAMMER:          Alex Xia
 *
 * INTERFACE:           void SetFileName(QString fileName)
 *
 * RETURNS:             void
 *
 * NOTES:
 * Sets the fileName to the file selected by the user.
 ******************************************************************************/
void SettingsWindow::SetFileName(QString fileName)
{
    selectedFileName = fileName;
}
