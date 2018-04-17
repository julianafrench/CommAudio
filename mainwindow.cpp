#include "mainwindow.h"
#include "ui_mainwindow.h"
/******************************************************************************
 * SOURCE FILE: mainwindow.cpp
 *
 * PROGRAM:     CommAudio
 *
 * FUNCTIONS:
 *
 *             MainWindow::MainWindow(QWidget *parent) :
 *             void MainWindow::UpdateSelectedFile(int row, int column)
 *             void MainWindow::UpdatePlaylist(QString updatedPlaylist)
 *             void MainWindow::AlertWrongFileType()
 *             void MainWindow::UpdateSongProgress(qint64 position)
 *             void MainWindow::EnableConnect()
 *             void MainWindow::EnableDisconnect()
 *             void MainWindow::InitializeSongDuration(qint64 duration_ms)
 *             void MainWindow::ToggleStreaming(bool streamReady)
 *             MainWindow::~MainWindow()
 *             void MainWindow::UpdateSettings()
 *             QString MainWindow::loadPlaylist()
 *             void MainWindow::displayPlaylistByRow(int row)
 *             void MainWindow::displayFileSizeByRow(int row)
 *             void MainWindow::clearPlaylist()
 *             void MainWindow::on_actionServer_triggered()
 *             void MainWindow::on_actionClient_triggered()
 *             void MainWindow::on_actionConnect_triggered()
 *             void MainWindow::on_actionDisconnect_triggered()
 *             void MainWindow::on_SaveButton_clicked()
 *             void MainWindow::on_actionSettings_triggered()
 *             void MainWindow::UpdateReceiverStatus(QString msg)
 *             void MainWindow::UpdateSenderStatus(QString msg)
 *             void MainWindow::ShowFilePicker()
 *
 * DATE:      April 16 2018
 *
 * DESIGNER:  Luke Lee, Vafa Dehghan Saei, Alex Xia, Julianna French
 *
 * PROGRAMMER:Luke Lee, Vafa Dehghan Saei, Alex Xia, Julianna French
 *
 * NOTES:     This file will draw the main window and initialize the main functions
 *
 ******************************************************************************/
bool connected = false;

/******************************************************************************
 * FUNCTION:    MainWindow
 *
 * DATE:        April 16 2018
 *
 *
 * DESIGNER:    Vafa Dehghan Saei, Julianna French, Luke Lee, Alex Xia
 *
 * PROGRAMMER:  Vafa Dehghan Saei, Julianna French, Luke Lee, Alex Xia
 *
 * INTERFACE:   MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent), ui(new Ui::MainWindow)
 *
 * RETURNS:     N/A constructor
 *
 * NOTES:       This is the constructor of the MainWindow class.
 *              This constructor will setup the buttons and assign the respective functions to them.
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
 * FUNCTION:      UpdateSelectedFile
 *
 * DATE:          April 16 2018
 *
 *
 * DESIGNER:      Alex Xia
 *
 * PROGRAMMER:    Alex Xia
 *
 * INTERFACE:     void MainWindow::UpdateSelectedFile(int row, int column)
 *                    int column: The column of the song
 *                    int row:    The row of the song.
 *
 * RETURNS:       void
 *
 * NOTES:       This function will get the song the user clicked on in the menu.
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
 * FUNCTION:      UpdatePlaylist
 *
 * DATE:          April 16 2018
 *
 *
 * DESIGNER:      Alex Xia, Vafa Dehghan Saei
 *
 * PROGRAMMER:    Alex Xia, Vafa Dehghan Saei
 *
 * INTERFACE:     void MainWindow::UpdatePlaylist(QString updatedPlaylist)
 *                    QString updatedPlaylist: The update playlist of available songs.
 *
 * RETURNS:       void
 *
 * NOTES:         This function will iterate through the files on the server and display them for the user.
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
 * FUNCTION:      AlertWrongFileType
 *
 * DATE:          April 16 2018
 *
 *
 * DESIGNER:      Luke Lee
 *
 * PROGRAMMER:    Luke Lee
 *
 * INTERFACE:     void MainWindow::AlertWrongFileType()
 *
 * RETURNS:       void
 *
 * NOTES:         This function will notify the user if they selected an incorrect file type.
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
 * FUNCTION:      UpdateSongProgress
 *
 * DATE:          April 16 2018
 *
 *
 * DESIGNER:      Julianna French
 *
 * PROGRAMMER:    Julianna French
 *
 * INTERFACE:     void MainWindow::UpdateSongProgress(qint64 position)
 *                    qint64 position: The position of the song
 *
 * RETURNS:       void
 *
 * NOTES:         This function will update the progress slider and text of the song.
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
 * FUNCTION:        EnableConnect
 *
 * DATE:            April 16 2018
 *
 *
 * DESIGNER:        Alex Xia
 *
 * PROGRAMMER:      Alex Xia
 *
 * INTERFACE:       void MainWindow::EnableConnect()
 *
 * RETURNS:         void
 *
 * NOTES:           This connection will enable the connect action and disable the disconnect action
 *
 ******************************************************************************/
void MainWindow::EnableConnect()
{
    ui->actionConnect->setEnabled(true);
    ui->actionDisconnect->setEnabled(false);
}

/******************************************************************************
 * FUNCTION:        EnableDisconnect
 *
 * DATE:            April 16 2018
 *
 *
 * DESIGNER:        Alex Xia
 *
 * PROGRAMMER:      Alex Xia
 *
 * INTERFACE:       void MainWindow::EnableDisconnect()
 *
 * RETURNS:         void
 *
 * NOTES:           This connection will disable the connect action and enable the disconnect action
 *
 ******************************************************************************/
void MainWindow::EnableDisconnect()
{
    ui->actionDisconnect->setEnabled(true);
    ui->actionConnect->setEnabled(false);
}

/******************************************************************************
 * FUNCTION:        InitializeSongDuration
 *
 * DATE:            April 16 2018
 *
 *
 * DESIGNER:        Vafa Dehghan Saei
 *
 * PROGRAMMER:      Vafa Dehghan Saei
 *
 * INTERFACE:       void MainWindow::InitializeSongDuration(qint64 duration_ms)
 *                      qint64 duration_ms: The duration of the song
 *
 * RETURNS:         void
 *
 * NOTES:           This function will initialize the song slider and text to 0.
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
 * FUNCTION:          ToggleStreaming
 *
 * DATE:              April 16 2018
 *
 *
 * DESIGNER:          Julianna French
 *
 * PROGRAMMER:        Julianna French
 *
 * INTERFACE:         void MainWindow::ToggleStreaming(bool streamReady)
 *                        bool streamReady: A boolean that represent if the stream is ready or not
 *
 * RETURNS:           void
 *
 * NOTES:             This function will update the UI to display if the audio is streaming or not
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
 * FUNCTION:          ~MainWindow
 *
 * DATE:              April 16 2018
 *
 *
 * DESIGNER:          Luke Lee
 *
 * PROGRAMMER:        Luke Lee
 *
 * INTERFACE:         MainWindow::~MainWindow()
 *
 * RETURNS:           N/A, destructor
 *
 * NOTES:             This function will quit the streaming thread and delete the ui and settings.
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
 * FUNCTION:          UpdateSettings
 *
 * DATE:              April 16 2018
 *
 *
 * DESIGNER:         Alex Xia
 *
 * PROGRAMMER:       Alex Xia
 *
 * INTERFACE:        void MainWindow::UpdateSettings()
 *
 * RETURNS:          void
 *
 * NOTES:            This function will check what type the Host is, and what mode the program is running in.
 *                   It will then update the UI accordingly.
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
 * FUNCTION:          loadPlaylist
 *
 * DATE:              April 16 2018
 *
 *
 * DESIGNER:          Alex Xia, Luke Lee
 *
 * PROGRAMMER:        Alex Xia, Luke Lee
 *
 * INTERFACE:         QString MainWindow::loadPlaylist()
 *
 * RETURNS:           QString: The name of the file and the size as a string
 *
 * NOTES:             This function will update the menu with the songs on the server.
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
 * FUNCTION:          displayPlaylistByRow
 *
 * DATE:              April 16 2018
 *
 *
 * DESIGNER:          Vafa Dehghan Saei
 *
 * PROGRAMMER:        Vafa Dehghan Saei
 *
 * INTERFACE:         void MainWindow::displayPlaylistByRow(int row)
 *                        int row: The row to add the song
 *
 * RETURNS:           void
 *
 * NOTES:             This function will add a new song to the table at the given row.
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
 * FUNCTION:          displayFileSizeByRow
 *
 * DATE:              April 16 2018
 *
 *
 * DESIGNER:          Julianna French
 *
 * PROGRAMMER:        Julianna French
 *
 * INTERFACE:         void MainWindow::displayFileSizeByRow(int row)
 *                          int row: the row to add the file size.
 *
 * RETURNS:           void
 *
 * NOTES:             This function will add the songs file size on the given row.
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
 * FUNCTION:        clearPlaylist
 *
 * DATE:            April 16 2018
 *
 *
 * DESIGNER:        Vafa Dehghan Saei
 *
 * PROGRAMMER:      Vafa Dehghan Saei
 *
 * INTERFACE:       void MainWindow::clearPlaylist()
 *
 * RETURNS:         void
 *
 * NOTES:           This function will clear the table of all songs.
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
 * FUNCTION:        on_actionServer_triggered
 *
 * DATE:            April 16 2018
 *
 *
 * DESIGNER:        Julianna French.
 *
 * PROGRAMMER:      Julianna French
 *
 * INTERFACE:       void MainWindow::on_actionServer_triggered()
 *
 * RETURNS:         void
 *
 * NOTES:           This function will set the hostType to server.
 *
 ******************************************************************************/
void MainWindow::on_actionServer_triggered()
{
    hostType = SERVER;
}

/******************************************************************************
 * FUNCTION:        on_actionClient_triggered
 *
 * DATE:            April 16 2018
 *
 *
 * DESIGNER:        Julianna French.
 *
 * PROGRAMMER:      Julianna French
 *
 * INTERFACE:       void MainWindow::on_actionClient_triggered()
 *
 * RETURNS:         void
 *
 * NOTES:           This function will set the hostType to client.
 *
 ******************************************************************************/
void MainWindow::on_actionClient_triggered()
{
    hostType = CLIENT;
}

/******************************************************************************
 * FUNCTION:        on_actionConnect_triggered
 *
 * DATE:            April 16 2018
 *
 *
 * DESIGNER:        Luke Lee
 *
 * PROGRAMMER:      Luke Lee
 *
 * INTERFACE:       void MainWindow::on_actionConnect_triggered()
 *
 * RETURNS:         void
 *
 * NOTES:           This function will check the host type and load the playlist if server, and connect the TransferModule.
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
 * FUNCTION:        on_actionDisconnect_triggered
 *
 * DATE:            April 16 2018
 *
 *
 * DESIGNER:        Luke Lee
 *
 * PROGRAMMER:      Luke Lee
 *
 * INTERFACE:       void MainWindow::on_actionDisconnect_triggered()
 *
 * RETURNS:         void
 *
 * NOTES:           This function will disconnect the TransferModule
 *
 ******************************************************************************/
void MainWindow::on_actionDisconnect_triggered()
{
    transferer->Disconnect();
}

/******************************************************************************
 * FUNCTION:        on_SaveButton_clicked
 *
 * DATE:            April 16 2018
 *
 *
 * DESIGNER:        Vafa Dehghan Saei, Julianna French
 *
 * PROGRAMMER:      Vafa Dehghan Saei, Julianna French
 *
 * INTERFACE:       void MainWindow::on_SaveButton_clicked()
 *
 * RETURNS:         void
 *
 * NOTES:           This function will save the song when the button is clicked.
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
 * FUNCTION:       on_actionSettings_triggered
 *
 * DATE:           April 16 2018
 *
 *
 * DESIGNER:      Alex Xia
 *
 * PROGRAMMER:    Alex Xia
 *
 * INTERFACE:     void MainWindow::on_actionSettings_triggered()
 *
 * RETURNS:       void
 *
 * NOTES:         This function will execute the settings window
 *
 ******************************************************************************/
void MainWindow::on_actionSettings_triggered()
{
    settings->exec();
}

/******************************************************************************
 * FUNCTION:        UpdateReceiverStatus
 *
 * DATE:            April 16 2018
 *
 *
 * DESIGNER:      Luke Lee
 *
 * PROGRAMMER:    Luke Lee
 *
 * INTERFACE:    void MainWindow::UpdateSenderStatus(QString msg)
 *                  QString msg: The string to update the receiver status to
 *
 * RETURNS:       void
 *
 * NOTES:       This function will update the receiver status text.
 *
 ******************************************************************************/
void MainWindow::UpdateReceiverStatus(QString msg)
{
    ui->ReceiverStatusLabel->setText(msg);
}

/******************************************************************************
 * FUNCTION:        UpdateSenderStatus
 *
 * DATE:            April 16 2018
 *
 *
 * DESIGNER:      Luke Lee
 *
 * PROGRAMMER:    Luke Lee
 *
 * INTERFACE:    void MainWindow::UpdateSenderStatus(QString msg)
 *                  QString msg: The string to update the server status to
 *
 * RETURNS:       void
 *
 * NOTES:       This function will update the server status text.
 *
 ******************************************************************************/
void MainWindow::UpdateSenderStatus(QString msg)
{
    ui->SenderStatusLabel->setText(msg);
}

/******************************************************************************
 * FUNCTION:          ShowFilePicker
 *
 * DATE:              April 16 2018
 *
 *
 * DESIGNER:          Julianna French
 *
 * PROGRAMMER:        Julianna French
 *
 * INTERFACE:         void MainWindow::ShowFilePicker()
 *
 * RETURNS:           void
 *
 * NOTES:             This function will display the file picker and allow the user to choose an mp3 or wav file.
 *
 ******************************************************************************/
void MainWindow::ShowFilePicker()
{
    QString newFileName = QFileDialog::getOpenFileName((QWidget*)this->parent(),
        tr("Choose another file to play/stream"), "", tr("(*.wav *.mp3)"));
    ui->CurrentSongLineEdit->setText(newFileName);
    settings->SetFileName(newFileName);
}
