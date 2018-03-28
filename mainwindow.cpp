#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "server.h"
#include "client.h"

using namespace commaudio;

// Global variables
ServerInfo *svrInfo;
ClientInfo *clntInfo;

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    hostType = -1;

    // connect menu items to actions
    //connect(ui->actionServer, &QAction::triggered, this, &MainWindow::on_actionServer_triggered);
}

MainWindow::~MainWindow()
{
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
    playlist = loadPlaylist();
    svrInfo = new ServerInfo();
    svrInfo->songlist = playlist.toStdString();
}

void MainWindow::on_actionClient_triggered()
{
    hostType = CLIENT;
    clntInfo = new ClientInfo();
    clntInfo->server_input = "127.0.0.1";
}

void MainWindow::on_actionConnect_triggered()
{
    if (hostType = SERVER)
    {
        Server::SvrConnect(svrInfo);
    }
    else if (hostType = CLIENT)
    {
        Client::ClntConnect(clntInfo);
        playlist = QString::fromStdString(clntInfo->songlist);

        ui->listWidget->addItems(playlist.split(";"));
        ui->listWidget->setCurrentRow(0);
    }
}
