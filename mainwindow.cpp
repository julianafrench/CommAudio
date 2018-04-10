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

    settings = new SettingsWindow(parent);
    connect(settings, &QDialog::accepted, this, &MainWindow::UpdateSettings);

    connect(ui->menuAbout, &QMenu::aboutToHide, this, &MainWindow::on_actionAbout_triggered);
    connect(ui->menuHelp, &QMenu::aboutToHide, this, &MainWindow::on_actionHelp_triggered);

    streamer = new StreamingModule(this, settings);
    connect(ui->StartSpeakerButton, &QPushButton::pressed, streamer, &StreamingModule::StartReceiver);
    connect(ui->StartMicButton, &QPushButton::pressed, streamer, &StreamingModule::AttemptStreamConnect);
    connect(streamer, &StreamingModule::ReceiverStatusUpdated, this, &MainWindow::UpdateReceiverStatus);
    connect(streamer, &StreamingModule::SenderStatusUpdated, this, &MainWindow::UpdateSenderStatus);

    connect(ui->StreamDisconnectButton, &QPushButton::pressed, streamer, &StreamingModule::AttemptStreamDisconnect);
    connect(streamer, &StreamingModule::ReceiverReady, this, &MainWindow::ToggleStreaming);
    ToggleStreaming(false);
    UpdateSettings();
    // connect menu items to actions
    //connect(ui->actionServer, &QAction::triggered, this, &MainWindow::on_actionServer_triggered);
    //connect(ui->saveBtn, &QPushButton::clicked, this, &MainWindow::on_actionServer_triggered);
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
    ui->listWidget->clear();

    QDir directory(QDir::currentPath());
    QStringList fileFilter("*.wav");

    QStringList files = directory.entryList(fileFilter);

    ui->listWidget->addItems(files);
    ui->listWidget->setCurrentRow(0);

    QString fileStr = files.join(";");

    return fileStr;
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
            Client::ReceivePlaylist();
            playlist = QString::fromStdString(clntInfo->songlist);

            if (playlist != "")
            {
                ui->listWidget->addItems(playlist.split(";"));
                ui->listWidget->setCurrentRow(0);
            }
            else
            {
                // warning dialog for no audio files found
            }
        }
        ui->actionConnect->setEnabled(false);
    }
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
    if(ui->listWidget->count() != 0)
    {
        return ui->listWidget->currentItem()->text();
        //qDebug("selected: " + filename);
    }
    return "";
}

void MainWindow::on_saveBtn_clicked()
{
    if (hostType == CLIENT)
    {
        QString filename = getSelectedFile();
        if(Client::SendFilename(filename.toStdString()))
        {
            Client::ReceiveFileSetup();
        }
    }
}

void MainWindow::on_actionSettings_triggered()
{
    settings->exec();
}

void MainWindow::ToggleClientServerUi()
{
    //nothing yet, stuff to come?
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
