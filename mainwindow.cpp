#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "server.h"
#include "client.h"

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
    hostType = -1;

    // connect menu items to actions
    //connect(ui->actionServer, &QAction::triggered, this, &MainWindow::on_actionServer_triggered);
    //connect(ui->saveBtn, &QPushButton::clicked, this, &MainWindow::on_actionServer_triggered);
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
    clntInfo->server_input = "127.0.0.1";
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
