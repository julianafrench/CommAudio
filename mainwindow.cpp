#include "mainwindow.h"
#include "ui_mainwindow.h"
/******************************************************************************
 * SOURCE FILE:
 *
 * PROGRAM:
 *
 * FUNCTIONS:
 *
 * DATE:
 *
 * REVISIONS:
 *
 * DESIGNER:
 *
 * PROGRAMMER:
 *
 * NOTES:
 *
 ******************************************************************************/
bool connected = false;

/******************************************************************************
 * FUNCTION:
 *
 * DATE:
 *
 * REVISIONS:
 *
 * DESIGNER:
 *
 * PROGRAMMER:
 *
 * INTERFACE:
 *
 * RETURNS:
 *
 * NOTES:
 *
 ******************************************************************************/
MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    hostType = UNDEFINED;

    // table widget setup
    ui->tableWidget->setColumnCount(2);
    ui->tableWidget->setColumnWidth(0, this->width() * 0.75);
    ui->tableWidget->setColumnWidth(1, this->width() * 0.22);
    ui->tableWidget->setSelectionMode(QAbstractItemView::SingleSelection);
    ui->tableWidget->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    connect(ui->tableWidget, &QTableWidget::cellClicked, this, &MainWindow::UpdateSelectedFile);

    // settings setup
    settings = new SettingsWindow(parent);
    connect(settings, &QDialog::accepted, this, &MainWindow::UpdateSettings);
    connect(ui->FilePickerButton, &QPushButton::clicked, this, &MainWindow::ShowFilePicker);

    // streaming setup
    streamer = new StreamingModule;
    streamer->settings = settings;
    streamer->moveToThread(&streamingThread);
    connect(&streamingThread, &QThread::finished, streamer, &QObject::deleteLater);
    streamingThread.start();
    // streaming slots
    connect(ui->StartSpeakerButton, &QPushButton::clicked, streamer, &StreamingModule::StartReceiver);
    connect(ui->StartMicButton, &QPushButton::clicked, streamer, &StreamingModule::AttemptStreamConnect);
    connect(streamer, &StreamingModule::ReceiverStatusUpdated, this, &MainWindow::UpdateReceiverStatus);
    connect(streamer, &StreamingModule::SenderStatusUpdated, this, &MainWindow::UpdateSenderStatus);
    connect(ui->StreamDisconnectButton, &QPushButton::clicked, streamer, &StreamingModule::AttemptStreamDisconnect);
    connect(streamer, &StreamingModule::ReceiverReady, this, &MainWindow::ToggleStreaming);
    connect(streamer, &StreamingModule::WrongFileType, this, &MainWindow::AlertWrongFileType);
    connect(ui->MulticastButton, &QPushButton::clicked, streamer, &StreamingModule::MulticastAudioInput);
    UpdateSettings();

    // media player setup
    mediaPlayer = new MediaPlayerModule;
    mediaPlayer->settings = settings;
    connect(ui->PlayButton, &QPushButton::clicked, mediaPlayer, &MediaPlayerModule::Play);
    connect(ui->PauseButton, &QPushButton::clicked, mediaPlayer, &MediaPlayerModule::Pause);
    connect(ui->StopButton, &QPushButton::clicked, mediaPlayer, &MediaPlayerModule::Stop);
    connect(ui->FastForwardButton, &QPushButton::clicked, mediaPlayer, &MediaPlayerModule::FastForward);
    connect(ui->SlowForwardButton, &QPushButton::clicked, mediaPlayer, &MediaPlayerModule::SlowForward);
    connect(mediaPlayer->player, &QMediaPlayer::durationChanged, this, &MainWindow::InitializeSongDuration);
    connect(mediaPlayer->player, &QMediaPlayer::positionChanged, this, &MainWindow::UpdateSongProgress);
    connect(ui->SongProgressSlider, &QSlider::sliderMoved, mediaPlayer, &MediaPlayerModule::ChangeSongPosition);

    // file transferer setup
    transferer = new TransferModule;
    transferer->settings = settings;
    connect(transferer, &TransferModule::Connected, this, &MainWindow::EnableDisconnect);
    connect(transferer, &TransferModule::Disconnected, this, &MainWindow::EnableConnect);
    connect(transferer, &TransferModule::ReceiverStatusUpdated, this, &MainWindow::UpdateReceiverStatus);
    connect(transferer, &TransferModule::SenderStatusUpdated, this, &MainWindow::UpdateSenderStatus);
    connect(transferer, &TransferModule::PlaylistReady, this, &MainWindow::UpdatePlaylist);
    ui->actionConnect->setEnabled(true);
    ui->actionDisconnect->setEnabled(false);
}

/******************************************************************************
 * FUNCTION:
 *
 * DATE:
 *
 * REVISIONS:
 *
 * DESIGNER:
 *
 * PROGRAMMER:
 *
 * INTERFACE:
 *
 * RETURNS:
 *
 * NOTES:
 *
 ******************************************************************************/
void MainWindow::UpdateSelectedFile(int row, int column)
{
    column = 0;
    QString selectedFileName = ui->tableWidget->item(row, column)->text();
    ui->CurrentSongLineEdit->setText(selectedFileName);
    settings->SetFileName(selectedFileName);
}

/******************************************************************************
 * FUNCTION:
 *
 * DATE:
 *
 * REVISIONS:
 *
 * DESIGNER:
 *
 * PROGRAMMER:
 *
 * INTERFACE:
 *
 * RETURNS:
 *
 * NOTES:
 *
 ******************************************************************************/
void MainWindow::UpdatePlaylist(QString updatedPlaylist)
{
    clearPlaylist();
    if (updatedPlaylist != "")
    {
        QStringList fileInfo = updatedPlaylist.split("|");
        fileNames = fileInfo[0].split(";");
        fileSizes = fileInfo[1].split(";");
        //clntInfo->songSizes = fileSizes;

        for (int i = 0; i < fileNames.size(); i++)
        {
            ui->tableWidget->insertRow(i);
            // display file name
            displayPlaylistByRow(i);

            // display file size
            displayFileSizeByRow(i);
        }
        ui->tableWidget->setHorizontalHeaderLabels(QStringList() << "File Name" << "File Size (B)");
    }
    else
    {
        QMessageBox msgBox;
        msgBox.setText("No audio files on server!");
        msgBox.exec();
    }
}

/******************************************************************************
 * FUNCTION:
 *
 * DATE:
 *
 * REVISIONS:
 *
 * DESIGNER:
 *
 * PROGRAMMER:
 *
 * INTERFACE:
 *
 * RETURNS:
 *
 * NOTES:
 *
 ******************************************************************************/
void MainWindow::AlertWrongFileType()
{
    QMessageBox popup;
    popup.setText("Must select a .wav file for streaming!");
    popup.exec();
    return;
}

/******************************************************************************
 * FUNCTION:
 *
 * DATE:
 *
 * REVISIONS:
 *
 * DESIGNER:
 *
 * PROGRAMMER:
 *
 * INTERFACE:
 *
 * RETURNS:
 *
 * NOTES:
 *
 ******************************************************************************/
void MainWindow::UpdateSongProgress(qint64 position)
{
    qint64 totalSecondsPlayed = position / 1000;
    long minutes = totalSecondsPlayed / 60;
    long seconds = totalSecondsPlayed % 60;
    QString minutesStr = QString("%1").arg(minutes, 2, 10, QChar('0'));
    QString secondsStr = QString("%1").arg(seconds, 2, 10, QChar('0'));
    ui->SongProgressLabel->setText(minutesStr + ":" + secondsStr);
    ui->SongProgressSlider->setValue(totalSecondsPlayed);
}

/******************************************************************************
 * FUNCTION:
 *
 * DATE:
 *
 * REVISIONS:
 *
 * DESIGNER:
 *
 * PROGRAMMER:
 *
 * INTERFACE:
 *
 * RETURNS:
 *
 * NOTES:
 *
 ******************************************************************************/
void MainWindow::EnableConnect()
{
    ui->actionConnect->setEnabled(true);
    ui->actionDisconnect->setEnabled(false);
}

/******************************************************************************
 * FUNCTION:
 *
 * DATE:
 *
 * REVISIONS:
 *
 * DESIGNER:
 *
 * PROGRAMMER:
 *
 * INTERFACE:
 *
 * RETURNS:
 *
 * NOTES:
 *
 ******************************************************************************/
void MainWindow::EnableDisconnect()
{
    ui->actionDisconnect->setEnabled(true);
    ui->actionConnect->setEnabled(false);
}

/******************************************************************************
 * FUNCTION:
 *
 * DATE:
 *
 * REVISIONS:
 *
 * DESIGNER:
 *
 * PROGRAMMER:
 *
 * INTERFACE:
 *
 * RETURNS:
 *
 * NOTES:
 *
 ******************************************************************************/
void MainWindow::InitializeSongDuration(qint64 duration_ms)
{
    qint64 totalSeconds = duration_ms / 1000;
    long minutes = totalSeconds / 60;
    long seconds = totalSeconds % 60;
    QString minutesStr = QString("%1").arg(minutes, 2, 10, QChar('0'));
    QString secondsStr = QString("%1").arg(seconds, 2, 10, QChar('0'));
    ui->SongDurationLabel->setText(minutesStr + ":" + secondsStr);
    ui->SongProgressSlider->setMaximum(totalSeconds);
    ui->SongProgressLabel->setText("00:00");
}

/******************************************************************************
 * FUNCTION:
 *
 * DATE:
 *
 * REVISIONS:
 *
 * DESIGNER:
 *
 * PROGRAMMER:
 *
 * INTERFACE:
 *
 * RETURNS:
 *
 * NOTES:
 *
 ******************************************************************************/
void MainWindow::ToggleStreaming(bool streamReady)
{
    if (streamReady) //receiver set up, unconnected, allow 2 connect
    {
        ui->StartSpeakerButton->setEnabled(false);
        if (settings->GetHostMode() == "Client")
            ui->StartMicButton->setEnabled(true);
        ui->StreamDisconnectButton->setEnabled(true);
    }
    else
    {
        ui->StartSpeakerButton->setEnabled(true);
        ui->StartMicButton->setEnabled(false);
        ui->StreamDisconnectButton->setEnabled(false);
    }
}

/******************************************************************************
 * FUNCTION:
 *
 * DATE:
 *
 * REVISIONS:
 *
 * DESIGNER:
 *
 * PROGRAMMER:
 *
 * INTERFACE:
 *
 * RETURNS:
 *
 * NOTES:
 *
 ******************************************************************************/
MainWindow::~MainWindow()
{
    streamingThread.quit();
    streamingThread.wait();
    //streamer is connected to delete once thread finished
    delete settings;
    delete ui;
}

/******************************************************************************
 * FUNCTION:
 *
 * DATE:
 *
 * REVISIONS:
 *
 * DESIGNER:
 *
 * PROGRAMMER:
 *
 * INTERFACE:
 *
 * RETURNS:
 *
 * NOTES:
 *
 ******************************************************************************/
void MainWindow::UpdateSettings()
{
    if(settings->GetHostMode() == "Client")
    {
        on_actionClient_triggered();
    }

    if(settings->GetHostMode() == "Server")
    {
        on_actionServer_triggered();
    }

    if (settings->GetTransferMode() == "file transfer")
    {
        ui->StartSpeakerButton->setDisabled(true);
        ui->StartMicButton->setDisabled(true);
        ui->StreamDisconnectButton->setDisabled(true);
    }
    else
    {
        ui->StartSpeakerButton->setEnabled(true);
    }
    if (settings->GetTransferMode() == "multicast" && settings->GetHostMode() == "Server")
    {
        ui->MulticastButton->setEnabled(true);
    }
    else
    {
        ui->MulticastButton->setDisabled(true);
    }
}

/******************************************************************************
 * FUNCTION:
 *
 * DATE:
 *
 * REVISIONS:
 *
 * DESIGNER:
 *
 * PROGRAMMER:
 *
 * INTERFACE:
 *
 * RETURNS:
 *
 * NOTES:
 *
 ******************************************************************************/
QString MainWindow::loadPlaylist()
{
    clearPlaylist();
    QString fileStr = "";
    QDir directory(QDir::currentPath());
    QStringList audioFileFilter;
    audioFileFilter << "*.wav" << "*.mp3";
    directory.setNameFilters(audioFileFilter);
    fileNames = directory.entryList();
    if (fileNames.size() == 0)
    {
        QMessageBox popup;
        popup.setText("No audio files found in current directory!");
        popup.exec();
        return "";
    }
    for (int i = 0; i < fileNames.size(); i++)
    {
        ui->tableWidget->insertRow(i);

        // display file name
        displayPlaylistByRow(i);

        // display file size
        QFileInfo fInfo(fileNames[i]);
        fileSizes.append(QString::number(fInfo.size()));
        displayFileSizeByRow(i);
    }

    QString nameStr = fileNames.join(";");
    QString sizeStr = fileSizes.join(";");
    fileStr = nameStr + "|" + sizeStr;

    ui->tableWidget->setHorizontalHeaderLabels(QStringList() << "File Name" << "File Size (B)");
    return fileStr;
}

/******************************************************************************
 * FUNCTION:
 *
 * DATE:
 *
 * REVISIONS:
 *
 * DESIGNER:
 *
 * PROGRAMMER:
 *
 * INTERFACE:
 *
 * RETURNS:
 *
 * NOTES:
 *
 ******************************************************************************/
void MainWindow::displayPlaylistByRow(int row)
{
    QTableWidgetItem *fName = new QTableWidgetItem;
    fName->setText(fileNames[row]);
    // disable editing
    fName->setFlags(fName->flags() & ~Qt::ItemIsEditable);
    ui->tableWidget->setItem(row, 0, fName);
}

/******************************************************************************
 * FUNCTION:
 *
 * DATE:
 *
 * REVISIONS:
 *
 * DESIGNER:
 *
 * PROGRAMMER:
 *
 * INTERFACE:
 *
 * RETURNS:
 *
 * NOTES:
 *
 ******************************************************************************/
void MainWindow::displayFileSizeByRow(int row)
{
    QTableWidgetItem *fSize = new QTableWidgetItem;
    fSize->setText(fileSizes[row]);
    fSize->setFlags(Qt::NoItemFlags);
    ui->tableWidget->setItem(row, 1, fSize);
}

/******************************************************************************
 * FUNCTION:
 *
 * DATE:
 *
 * REVISIONS:
 *
 * DESIGNER:
 *
 * PROGRAMMER:
 *
 * INTERFACE:
 *
 * RETURNS:
 *
 * NOTES:
 *
 ******************************************************************************/
void MainWindow::clearPlaylist()
{
    ui->tableWidget->setRowCount(0);
    fileNames.clear();
    fileSizes.clear();
    ui->tableWidget->setHorizontalHeaderLabels(QStringList() << "File Name" << "File Size (B)");
}

/******************************************************************************
 * FUNCTION:
 *
 * DATE:
 *
 * REVISIONS:
 *
 * DESIGNER:
 *
 * PROGRAMMER:
 *
 * INTERFACE:
 *
 * RETURNS:
 *
 * NOTES:
 *
 ******************************************************************************/
void MainWindow::on_actionServer_triggered()
{
    hostType = SERVER;
}

/******************************************************************************
 * FUNCTION:
 *
 * DATE:
 *
 * REVISIONS:
 *
 * DESIGNER:
 *
 * PROGRAMMER:
 *
 * INTERFACE:
 *
 * RETURNS:
 *
 * NOTES:
 *
 ******************************************************************************/
void MainWindow::on_actionClient_triggered()
{
    hostType = CLIENT;
}

/******************************************************************************
 * FUNCTION:
 *
 * DATE:
 *
 * REVISIONS:
 *
 * DESIGNER:
 *
 * PROGRAMMER:
 *
 * INTERFACE:
 *
 * RETURNS:
 *
 * NOTES:
 *
 ******************************************************************************/
void MainWindow::on_actionConnect_triggered()
{
    if (hostType == SERVER)
    {
        QString playlistLine = loadPlaylist();        
        transferer->Connect(playlistLine);
    }
    else if (hostType == CLIENT)
    {
        transferer->Connect();
    }
}

/******************************************************************************
 * FUNCTION:
 *
 * DATE:
 *
 * REVISIONS:
 *
 * DESIGNER:
 *
 * PROGRAMMER:
 *
 * INTERFACE:
 *
 * RETURNS:
 *
 * NOTES:
 *
 ******************************************************************************/
void MainWindow::on_actionDisconnect_triggered()
{
    transferer->Disconnect();
}

/******************************************************************************
 * FUNCTION:
 *
 * DATE:
 *
 * REVISIONS:
 *
 * DESIGNER:
 *
 * PROGRAMMER:
 *
 * INTERFACE:
 *
 * RETURNS:
 *
 * NOTES:
 *
 ******************************************************************************/
void MainWindow::on_SaveButton_clicked()
{
    if (hostType == CLIENT)
    {
        if (ui->tableWidget->rowCount() != 0)
        {
            QString filename = ui->tableWidget->currentItem()->text();
            if (filename == "")
            {
                QMessageBox msgBox;
                msgBox.setText("No audio file chosen!");
                msgBox.exec();
            }
            transferer->SongSelected(filename);
        }
    }
}

/******************************************************************************
 * FUNCTION:
 *
 * DATE:
 *
 * REVISIONS:
 *
 * DESIGNER:
 *
 * PROGRAMMER:
 *
 * INTERFACE:
 *
 * RETURNS:
 *
 * NOTES:
 *
 ******************************************************************************/
void MainWindow::on_actionSettings_triggered()
{
    settings->exec();
}

/******************************************************************************
 * FUNCTION:
 *
 * DATE:
 *
 * REVISIONS:
 *
 * DESIGNER:
 *
 * PROGRAMMER:
 *
 * INTERFACE:
 *
 * RETURNS:
 *
 * NOTES:
 *
 ******************************************************************************/
void MainWindow::UpdateReceiverStatus(QString msg)
{
    ui->ReceiverStatusLabel->setText(msg);
}

/******************************************************************************
 * FUNCTION:
 *
 * DATE:
 *
 * REVISIONS:
 *
 * DESIGNER:
 *
 * PROGRAMMER:
 *
 * INTERFACE:
 *
 * RETURNS:
 *
 * NOTES:
 *
 ******************************************************************************/
void MainWindow::UpdateSenderStatus(QString msg)
{
    ui->SenderStatusLabel->setText(msg);
}

/******************************************************************************
 * FUNCTION:
 *
 * DATE:
 *
 * REVISIONS:
 *
 * DESIGNER:
 *
 * PROGRAMMER:
 *
 * INTERFACE:
 *
 * RETURNS:
 *
 * NOTES:
 *
 ******************************************************************************/
void MainWindow::ShowFilePicker()
{
    QString newFileName = QFileDialog::getOpenFileName((QWidget*)this->parent(),
        tr("Choose another file to play/stream"), "", tr("(*.wav *.mp3)"));
    ui->CurrentSongLineEdit->setText(newFileName);
    settings->SetFileName(newFileName);
}
