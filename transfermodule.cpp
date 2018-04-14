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
    transmitting = false;
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
    if (transmitting)
    {
        QString fileToDownload = fileToTransfer;
        QFile file(fileToDownload);
        file.open(QIODevice::WriteOnly | QIODevice::Append);
        QDataStream fileWriteStream(&file);
        QByteArray fileBytes = ioSocket->readAll();
        //QByteArray default max size ~8188 bytes/chars
        //hex length will be less than int max value, ~32768
        std::string hex = fileBytes.toStdString();
        fileWriteStream.writeRawData(hex.c_str(), hex.length()); //this should be ~8188, way less than max int value
        fileSize -= hex.length();
        if (fileSize <= 0)
        {
            QMessageBox popup;
            popup.setText(fileToDownload + " is done!");
            popup.exec();
            transmitting = false;
        }
        file.close();
        return;
    }
    ioSocket->read(4); //these consist of \0 \0 \0 '
    QByteArray descriptorBytes = ioSocket->read(8); //idk y 12 bytes = 8 chars
    QString descriptor(descriptorBytes);
    if (descriptor == "filelist")
    {
        QByteArray fileNamesBytes = ioSocket->readAll();
        QString fileNames = QString(fileNamesBytes);
        emit PlaylistReady(fileNames);
    }
    if (descriptor == "filesize")
    {
        QByteArray fileSizeBytes = ioSocket->readAll();
        fileSize = fileSizeBytes.toLong();
        QString fileSizeStr = QString::number(fileSize);
        fileSizeBytes = descriptorBytes + fileSizeStr.toUtf8();
        QDataStream socketWriteStream(ioSocket);
        socketWriteStream << fileSizeBytes;
    }
    if (descriptor == "filebyte")
    {
        transmitting = true;
        QString fileToDownload = fileToTransfer;
        QFile file(fileToDownload);
        if(file.open(QIODevice::WriteOnly))
        {
            emit ReceiverStatusUpdated("Client: connected & saving to file");
            QDataStream fileWriteStream(&file);
            QByteArray fileBytes = ioSocket->readAll();
            std::string hex = fileBytes.toStdString();
            fileWriteStream.writeRawData(hex.c_str(), hex.length());
            fileSize -= hex.length();
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
        // store file name & send filesize back
        QByteArray fileNameBytes = ioSocket->readAll();
        QString fileToSend = QString(fileNameBytes);
        QFile file(fileToSend);
        if(file.open(QIODevice::ReadOnly))
        {
            nextFileToSend = fileToSend;
            fileSize = file.size();
            QString fileSizeStr = QString("filesize") + QString::number(file.size());
            QByteArray fileSizeBytes = fileSizeStr.toUtf8();
            QDataStream socketWriteStream(ioSocket);
            socketWriteStream << fileSizeBytes;
            emit SenderStatusUpdated("Sender: connected & sending");
        } else {
            emit SenderStatusUpdated("Sender: failed to open file " + fileToSend + ": " + file.errorString());
        }
    }
    if (descriptor == "filesize")
    {
        // verify filesize & send bytes back if correct
        QByteArray fileSizeBytes = ioSocket->readAll();
        long clientReceivedFileSize = fileSizeBytes.toLong();
        if (clientReceivedFileSize == fileSize)
        {
            QFile file(nextFileToSend);
            if(file.open(QIODevice::ReadOnly))
            {
                descriptorBytes = QString("filebyte").toUtf8();
                QDataStream socketWriteStream(ioSocket);
                QByteArray fileBytes = descriptorBytes + file.readAll();
                socketWriteStream << fileBytes;
                file.close();
                emit SenderStatusUpdated("Sender: connected & sending");
            } else {
               emit SenderStatusUpdated("Sender: failed to open file " + nextFileToSend + ": " + file.errorString());
            }
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
