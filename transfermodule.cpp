#include "transfermodule.h"

TransferModule::TransferModule(QObject *parent) : QObject(parent)
{
    // only used in server mode
    receiver = nullptr;
    // can use just 1 socket 4 read & write ,doesnt need 100% capacity like streaming
    ioSocket = nullptr;
}

TransferModule::~TransferModule()
{
    delete ioSocket;
    delete receiver;
}

void TransferModule::Connect(QString playlist)
{
    if (settings->GetHostMode() == "Client")
    {
        if (ioSocket == nullptr)
            ioSocket = new QTcpSocket;
        connect(ioSocket, &QTcpSocket::connected, this, &TransferModule::HandleConnect);
        connect(ioSocket, &QTcpSocket::disconnected, this, &TransferModule::HandleDisconnect);
        connect(ioSocket, &QTcpSocket::readyRead, this, &TransferModule::ClientReceivedBytes);
        ioSocket->connectToHost(settings->GetIpAddress(), DEF_PORT);
    }
    if (settings->GetHostMode() == "Server")
    {
        if (receiver == nullptr)
            receiver = new QTcpServer;
        delete ioSocket;
        connect(receiver, &QTcpServer::newConnection, this, &TransferModule::ClientConnected);
        if(!receiver->listen(QHostAddress::Any, DEF_PORT))
        {
            emit SenderStatusUpdated("Error: " + receiver->errorString());
        }
        else
        {
            playlistToSend = playlist;
            emit SenderStatusUpdated("Receiver: running");
            emit Connected();
        }
    }
}

void TransferModule::Disconnect()
{
    if (ioSocket)
        ioSocket->close();
    if (receiver)
        receiver->close();
    emit Disconnected();
}

void TransferModule::HandleConnect()
{
    emit Connected();
}

void TransferModule::HandleDisconnect()
{
    Disconnect();
    emit Disconnected();
}

void TransferModule::ClientReceivedBytes()
{
    ioSocket->read(4); //these consist of \0 \0 \0 '
    QByteArray descriptorBytes = ioSocket->read(8); //idk y 12 bytes = 8 chars
    QString descriptor(descriptorBytes);
    if (descriptor == "filelist")
    {
        QByteArray fileNamesBytes = ioSocket->readAll();
        QString fileNames = QString(fileNamesBytes);
        emit PlaylistReady(fileNames);
    }
    if (descriptor == "filebyte")
    {
        QString fileToDownload = fileToTransfer;
        QFile file(fileToDownload);
        if(file.open(QIODevice::WriteOnly | QIODevice::Append))
        {
            emit ReceiverStatusUpdated("Client: connected & saving to file");
            QDataStream fileWriteStream(&file);
            QByteArray fileBytes = ioSocket->readAll();
            fileWriteStream << fileBytes;
            file.close();
        } else {
           emit ReceiverStatusUpdated("Client: failed to open file " + fileToDownload + ": " + file.errorString());
        }
    }
}

void TransferModule::SongSelected(QString songName)
{
    fileToTransfer = songName;

    QString fileName = "filename";
    fileName += songName;
    QByteArray fileBytes = fileName.toUtf8();
    QDataStream socketWriteStream(ioSocket);
    socketWriteStream << fileBytes;
}

/*
    filename
    filelist
    filebyte
*/
void TransferModule::ServerReceivedBytes()
{
    ioSocket->read(4);
    QByteArray descriptorBytes = ioSocket->read(8);
    QString descriptor(descriptorBytes);
    if (descriptor == "filename")
    {
        QByteArray fileNameBytes = ioSocket->readAll();
        QString fileToSend = QString(fileNameBytes);

        QFile file(fileToSend);
        if(file.open(QIODevice::ReadOnly))
        {
            descriptorBytes = QString("filebyte").toUtf8();
            QDataStream socketWriteStream(ioSocket);
            socketWriteStream << descriptorBytes;
            QByteArray fileBytes = file.readAll();
            qDebug() << fileBytes.size();
            socketWriteStream << fileBytes;
            file.close();
           emit SenderStatusUpdated("Sender: connected & sending");
        } else {
           emit SenderStatusUpdated("Sender: failed to open file " + fileToSend + ": " + file.errorString());
        }
    }
}

//only called as server
void TransferModule::ClientConnected()
{
    ioSocket = receiver->nextPendingConnection();
    emit ReceiverStatusUpdated("Server: a new client connected.");
    connect(ioSocket, &QTcpSocket::disconnected, this, &TransferModule::HandleDisconnect);
    connect(ioSocket, &QTcpSocket::readyRead, this, &TransferModule::ServerReceivedBytes);
    QString fileNames = "filelist";
    fileNames += playlistToSend;
    QByteArray fileBytes = fileNames.toUtf8();
    QDataStream socketWriteStream(ioSocket);
    socketWriteStream << fileBytes;
}
