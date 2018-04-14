#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "server.h"
#include "client.h"
#include "helpwindow.h"
#include "aboutwindow.h"
using namespace commaudio;

// Function declarations (not in mainwindow.h)
DWORD WINAPI serverThread(LPVOID svrInfo);

// Global variables
ServerInfo *svrInfo;
ClientInfo *clntInfo;
bool connected = false;

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    hostType = UNDEFINED;

    // table widget setup
    ui->tableWidget->setColumnCount(2);
    ui->tableWidget->setColumnWidth(0, this->width() * 0.7);
    ui->tableWidget->setColumnWidth(1, this->width() * 0.2);
    ui->tableWidget->setSelectionMode(QAbstractItemView::SingleSelection);

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
        ui->StreamDisconnectButton->setEnabled(true);
    }
    else
    {
        ui->StartSpeakerButton->setEnabled(true);
        ui->StartMicButton->setEnabled(false);
        ui->StreamDisconnectButton->setEnabled(false);
    }
}

MainWindow::~MainWindow()
{
    if (svrInfo != nullptr)
    {
        free(svrInfo);
    }
    if (clntInfo != nullptr)
    {
        free(clntInfo);
    }
    streamingThread.quit();
    streamingThread.wait();
    delete settings;
    //streamer is connected to delete once thread finished
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
    else
    {
        ui->StartSpeakerButton->setEnabled(false);
    }

}

void MainWindow::on_actionExit_triggered()
{
    close();
}

QString MainWindow::loadPlaylist()
{
    clearPlaylist();
    QString fileStr = "";

    if (hostType == SERVER)
    {
        QDir directory(QDir::currentPath());
        QStringList fileFilter("*.wav");

        fileNames = directory.entryList(fileFilter);

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
    }
    else if (hostType == CLIENT)
    {
        QStringList fileInfo = playlist.split("|");
        fileNames = fileInfo[0].split(";");
        fileSizes = fileInfo[1].split(";");
        clntInfo->songSizes = fileSizes;

        for (int i = 0; i < fileNames.size(); i++)
        {
            ui->tableWidget->insertRow(i);
            // display file name
            displayPlaylistByRow(i);

            // display file size
            displayFileSizeByRow(i);
        }
    }
    ui->tableWidget->setHorizontalHeaderLabels(QStringList() << "File Name" << "File Size (B)");
    return fileStr;
}

void MainWindow::displayPlaylistByRow(int row)
{
    QTableWidgetItem *fName = new QTableWidgetItem;
    fName->setText(fileNames[row]);
    // disable editing
    fName->setFlags(fName->flags() & ~Qt::ItemIsEditable);
    ui->tableWidget->setItem(row, 0, fName);
}

void MainWindow::displayFileSizeByRow(int row)
{
    QTableWidgetItem *fSize = new QTableWidgetItem;
    fSize->setText(fileSizes[row]);
    fSize->setFlags(Qt::NoItemFlags);
    ui->tableWidget->setItem(row, 1, fSize);
}

void MainWindow::on_actionServer_triggered()
{
    hostType = SERVER;
    svrInfo = new ServerInfo();
}

void MainWindow::on_actionClient_triggered()
{
    hostType = CLIENT;
    clntInfo = new ClientInfo();
    clntInfo->server_input = settings->GetIpAddress().toStdString().c_str();
}

void MainWindow::on_actionConnect_triggered()
{
    if (hostType == SERVER)
    {
        playlist = loadPlaylist();
        svrInfo->songlist = playlist.toStdString();

        // create server thread
        DWORD svrThrdID;
        if (CreateThread(NULL, 0, serverThread, (LPVOID)svrInfo, 0, &svrThrdID) == NULL)
        {
            qWarning() << "failed to create server thread";
        }
        ui->actionConnect->setEnabled(false);
    }
    else if (hostType == CLIENT)
    {
        if(Client::ClntConnect(clntInfo))
        {
            connected = true;
            clntInfo->connected = &connected;
            //qDebug("Clnt with socket " + clntInfo->sendSocket + " connected");
            Client::ReceiveFileInfo();
            playlist = QString::fromStdString(clntInfo->songlist);
            //Client::ReceiveFileSizeList();

            if (playlist != "")
            {
                loadPlaylist();
            }
            else
            {
                // warning dialog for no audio files found
            }
        }
        ui->actionConnect->setEnabled(false);
    }
}

void MainWindow::on_actionDisconnect_triggered()
{
    qDebug() << "disconnected.";
    if (hostType == SERVER)
    {
        Server::Disconnect();
    }
    else if (hostType == CLIENT)
    {
        Client::Disconnect();
    }
    clearPlaylist();
    ui->actionConnect->setEnabled(true);
}

void MainWindow::clearPlaylist()
{
    ui->tableWidget->clear();
    fileNames.clear();
    fileSizes.clear();
    ui->tableWidget->setHorizontalHeaderLabels(QStringList() << "File Name" << "File Size (B)");
}

DWORD WINAPI serverThread(LPVOID svrInfo)
{
    ServerInfo *sInfo = (ServerInfo *)svrInfo;
    connected = Server::SvrConnect(sInfo);
    qDebug() << "server connected";
    sInfo->connected = &connected;
    while (connected)
    {
        Server::AcceptNewEvent();
    }
    return TRUE;
}

QString MainWindow::getSelectedFile()
{
    if(ui->tableWidget->rowCount() != 0)
    {
        if (ui->tableWidget->currentColumn() == 0)
        {
            bool converted = false;
            int row = ui->tableWidget->currentRow();
            QString str = ui->tableWidget->item(row, 1)->text();
            clntInfo->selFileSize = str.toLong(&converted, 10);
            return ui->tableWidget->currentItem()->text();
        }
        else
        {
            // display warning saying please select file name.
        }
    }
    return "";
}

void MainWindow::on_SaveButton_clicked()
{
    if (hostType == CLIENT)
    {
        clntInfo->saveDone = false;
        QString filename = getSelectedFile();
        if(Client::SendFilename(filename.toStdString()))
        {
            //UpdateReceiverStatus("Transfering file: " + filename);
            Client::ReceiveFileSetup();
        }
        while (!clntInfo->saveDone) {}
        //UpdateReceiverStatus("Transfer finished.");
        msgBox.setText("File transfer completed: " + filename);
        msgBox.exec();
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
