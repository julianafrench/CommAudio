#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "helpwindow.h"
#include "aboutwindow.h"

bool connected = false;

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    hostType = UNDEFINED;

    // settings setup
    settings = new SettingsWindow(parent);
    connect(settings, &QDialog::accepted, this, &MainWindow::UpdateSettings);
    connect(ui->menuAbout, &QMenu::aboutToHide, this, &MainWindow::on_actionAbout_triggered);
    connect(ui->menuHelp, &QMenu::aboutToHide, this, &MainWindow::on_actionHelp_triggered);
    UpdateSettings();

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
    ToggleStreaming(false);

    // media player setup
    mediaPlayer = new MediaPlayerModule;
    connect(ui->FilePickerButton, &QPushButton::clicked, mediaPlayer, &MediaPlayerModule::ShowFilePicker);
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

void MainWindow::UpdatePlaylist(QString updatedPlaylist)
{
    if (updatedPlaylist != "")
    {
        ui->listWidget->addItems(updatedPlaylist.split(";"));
        ui->listWidget->setCurrentRow(0);
    }
    else
    {
        QMessageBox msgBox;
        msgBox.setText("No audio files on server!");
        msgBox.exec();
    }
}


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

void MainWindow::EnableConnect()
{
    ui->actionConnect->setEnabled(true);
    ui->actionDisconnect->setEnabled(false);
}

void MainWindow::EnableDisconnect()
{
    ui->actionDisconnect->setEnabled(true);
    ui->actionConnect->setEnabled(false);
}

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

void MainWindow::ToggleStreaming(bool streamReady)
{
    if (streamReady) //receiver set up, unconnected, allow 2 connect
    {
        ui->StartSpeakerButton->setEnabled(false);
        if (settings->GetHostMode() == "Client")
            ui->StartMicButton->setEnabled(true);
//        ui->StreamDisconnectButton->setEnabled(true);
    }
    else
    {
        ui->StartSpeakerButton->setEnabled(true);
        ui->StartMicButton->setEnabled(false);
//        ui->StreamDisconnectButton->setEnabled(false);
    }
}

MainWindow::~MainWindow()
{
    streamingThread.quit();
    streamingThread.wait();
    //streamer is connected to delete once thread finished
    delete settings;
    delete ui;
}

void MainWindow::UpdateSettings()
{
    if(settings->GetHostMode() == "Client")
        on_actionClient_triggered();
    if(settings->GetHostMode() == "Server")
        on_actionServer_triggered();
    if(settings->GetTransferMode() == "microphone" || settings->GetTransferMode() == "streaming")
    {
        //mic will be enabled by speaker
        ui->StartSpeakerButton->setEnabled(true);
    }
    if (settings->GetTransferMode() == "file transfer")
    {
        ui->StartSpeakerButton->setEnabled(false);
    }
}

QString MainWindow::loadPlaylist()
{
    ui->listWidget->clear();
    QDir directory(QDir::currentPath());
    QStringList audioFileFilter;
    audioFileFilter << "*.wav" << "*.mp3";
    directory.setNameFilters(audioFileFilter);
    QStringList files = directory.entryList();

    ui->listWidget->addItems(files);
    ui->listWidget->setCurrentRow(0);

    return files.join(";");
}

void MainWindow::on_actionServer_triggered()
{
    hostType = SERVER;
}

void MainWindow::on_actionClient_triggered()
{
    hostType = CLIENT;
}

void MainWindow::on_actionConnect_triggered()
{
    if (hostType == SERVER)
    {
        QString playlistLine = loadPlaylist();
        transferer->Connect(playlistLine);
        ui->actionConnect->setEnabled(false);
    }
    else if (hostType == CLIENT)
    {
        transferer->Connect();
        ui->actionConnect->setEnabled(false);
    }
}

void MainWindow::on_actionDisconnect_triggered()
{
    transferer->Disconnect();
}

void MainWindow::on_SaveButton_clicked()
{
    if (hostType == CLIENT)
    {
        if(ui->listWidget->count() != 0)
        {
            QString filename = ui->listWidget->currentItem()->text();
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

void MainWindow::on_actionSettings_triggered()
{
    settings->exec();
}

void MainWindow::on_actionHelp_triggered()
{
    HelpWindow helpWindow;
    helpWindow.exec();
}

void MainWindow::on_actionAbout_triggered()
{
    AboutWindow aboutWindow;
    aboutWindow.exec();
}

void MainWindow::UpdateReceiverStatus(QString msg)
{
    ui->ReceiverStatusLabel->setText(msg);
}

void MainWindow::UpdateSenderStatus(QString msg)
{
    ui->SenderStatusLabel->setText(msg);
}
