#include "transfermodule.h"

/*------------------------------------------------------------------------------------------------------------------
-- SOURCE FILE: transfermodule.cpp - Handles all transfering functions
--
-- PROGRAM: CommAudio Networked Media Player
--
-- DATE: April 9, 2018
--
-- DESIGNER: Alex Xia, Juliana French, Luke Lee, Vafa Saei
--
-- PROGRAMMER: Alex Xia, Juliana French, Luke Lee
--
-- FUNCTIONS:
    TransferModule(QObject *parent = nullptr);
    ~TransferModule();
    void Connect(QString playlist = "");
    void Disconnect();
    void SongSelected(QString);
    void HandleConnect();
    void HandleDisconnect();
    void ClientReceivedBytes();
    void ServerReceivedBytes();
    void ClientConnected();
--
-- NOTES:
-- This class handles all transfering(downloading) functionality of the media player. For the most part, it is 
-- an isolated module, connected only to MainWindow by Qt signals; it connects on different ports than StreamingModule
-- so it can be ran together (cant be started together, as you still need to change the settings).
-- Like the other 2 modules(StreamingModule & MediaPlayerModule), it shares a pointer to a 
-- SettingsWindow object for getting user input/settings; 
-- 
-- After entering this mode by selecting transfer under settings & 
-- clicking connect, server & client do the following 
-- (better illustrated in the sequence diagram)
-- Client                                 Server
--   |                              scan all .wav & .mp3
--   |                              files in directory
--   |                              populate dropdown
--   |                              listen on port 6000
-- connect to given ip, port 6000  ----->   |             
-- recv                                     |
--   |    <-------------------------send file names (sizes) back       
-- populate songlist UI                    recv
-- user select song                         |
-- send file name     ------------------>   |
-- recv                                     |
--  |     <-------------------------send file size (type long) 
-- parse sizeof(long) file size            recv    
-- send file size back ----------------->   |
-- recv                                     |
--  |     <-------------------------if file size same as sent 
-- stop recv if:                    (security measure), send file bytes
--   - matching file size bytes recv'd
--   - other side disconnected  
--   - disconnected called by user
--
-- Each packet is prefixed by one of following 8 char/byte strings for its intended purpose:
--      filelist - sent by server, sends filelist back as single string to be delimited, eg. "filelistmysong.wav;mymixtap.mp3"
--      filebyte - sent by server, this & all subsequent packets will contains audio file bytes
--      filesize - sent by server, informs client after how many bytes to stop receiving eg. "filesize1002234"
--               - sent by client so server can verfify & start sending
--      filename - sent by client, requests a file, eg "filenamemylitmixtape.mp3" 
----------------------------------------------------------------------------------------------------------------------*/
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

/*------------------------------------------------------------------------------------------------------------------
-- FUNCTION Connect
--
-- DATE: April 13, 2018
--
-- DESIGNER: Alex Xia, Juliana French, Luke Lee, Vafa Saei
--
-- PROGRAMMER: Alex Xia, Juliana French
--
-- INTERFACE: void TransferModule::Connect(QString playlist)
--          playlist : a single line of file names joined with ';' eg "song1.mp3;song2.wav"  
--
-- RETURNS: void
--
-- NOTES:
-- Call this function to start a file download/transfer connection.
-- When called as client: will connect to an existing server with input from settings window.
-- When called as server, will start listening for client connections with input from settings window, and fills
-- the songlist TableWidget with audio files from current directory.
-- If no audio files are found, displays a popup.
-- Finally, updates some UI elements to show new status.
----------------------------------------------------------------------------------------------------------------------*/
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
        playlistToSend = playlist;
        if (playlistToSend.isEmpty())
        {
            emit SenderStatusUpdated("No audio to send");
            return;
        }
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
            emit SenderStatusUpdated("Server running");
            emit Connected();

        }
    }
}

/*------------------------------------------------------------------------------------------------------------------
-- FUNCTION Disconnect
--
-- DATE: April 13, 2018
--
-- DESIGNER: Alex Xia, Juliana French, Luke Lee, Vafa Saei
--
-- PROGRAMMER: Alex Xia, Juliana French
--
-- INTERFACE: void TransferModule::Disconnect()
--
-- RETURNS: void
--
-- NOTES:
-- Call this function to disconnect from a previous connection, and disable new connections until 
-- Connect is called again.
-- Closes and deletes socket involved in connection, as well as the listening server if one was created.
-- Finally, updates some UI elements to show new status.
----------------------------------------------------------------------------------------------------------------------*/
void TransferModule::Disconnect()
{
    if (ioSocket)
    {
        ioSocket->close();
        ioSocket->deleteLater();
    }
    if (receiver)
        receiver->close();
    transmitting = false;
    emit Disconnected();
    emit ReceiverStatusUpdated("Server has disconnected");
    emit SenderStatusUpdated("");
}

/*------------------------------------------------------------------------------------------------------------------
-- FUNCTION HandleConnect
--
-- DATE: April 13, 2018
--
-- DESIGNER: Alex Xia, Juliana French, Luke Lee, Vafa Saei
--
-- PROGRAMMER: Alex Xia, Juliana French
--
-- INTERFACE: void TransferModule::HandleConnect()
--
-- RETURNS: void
--
-- NOTES:
-- This function is triggered as a client once its socket connects to a server. It should not be called directly.
-- This function emits the Connected signal, which updates some UI elements in MainWindow.
-- As a server, the Connected signal is emitted as soon as listening is called, but clients will only emit Connected 
-- (by triggering this function) once its socket finished connecting with a server.
----------------------------------------------------------------------------------------------------------------------*/
void TransferModule::HandleConnect()
{
    emit Connected();
}

/*------------------------------------------------------------------------------------------------------------------
-- FUNCTION HandleDisconnect
--
-- DATE: April 13, 2018
--
-- DESIGNER: Alex Xia, Juliana French, Luke Lee, Vafa Saei
--
-- PROGRAMMER: Alex Xia, Juliana French
--
-- INTERFACE: void TransferModule::HandleConnect()
--
-- RETURNS: void
--
-- NOTES:
-- This function is triggered as a server once one of its client sockets disconnects. It should not be called directly.
-- Note that after a client disconnects, a server should stay running.
----------------------------------------------------------------------------------------------------------------------*/
void TransferModule::HandleDisconnect()
{
    emit ReceiverStatusUpdated("A client has disconnected");
    if (ioSocket)
    {
        ioSocket->close();
        ioSocket->deleteLater();
    }
}

/*------------------------------------------------------------------------------------------------------------------
-- FUNCTION ClientReceivedBytes
--
-- DATE: April 13, 2018
--
-- DESIGNER: Alex Xia, Juliana French, Luke Lee, Vafa Saei
--
-- PROGRAMMER: Alex Xia, Juliana French, Luke Lee
--
-- INTERFACE: void TransferModule::ClientReceivedBytes()
--
-- RETURNS: void
--
-- NOTES:
-- This function should not be called directly. It is triggered as a client when enough data arrives (max ~8188 bytes)
-- on its socket connected to the server. 
-- It will extract the first 8 bytes from the packet to see what is left on the socket to read:
--      filelist - a list of songs as a single string to be delimited, eg. "filelistmysong.wav;mymixtap.mp3"
--      filesize - sizeof a file in bytes to be downloaded
--      filebyte - this & all subsequent packets will contains audio file bytes
-- For filelist, it'll send it to MainWindow to populate songlist UI with.
-- For filesize, it'll send it back to server to request the file bytes.
-- For filebyte, it'll simply open & append the bytes to a file.
----------------------------------------------------------------------------------------------------------------------*/
void TransferModule::ClientReceivedBytes()
{
    if (transmitting)
    {
        QString fileToDownload = fileToTransfer;
        QFile file(fileToDownload);
        if(file.open(QIODevice::WriteOnly | QIODevice::Append))
        {
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
        } else {
            emit ReceiverStatusUpdated(file.errorString());
        }
        return;
    }
    ioSocket->read(4); //these consist of \0 \0 \0 '
    QByteArray descriptorBytes = ioSocket->read(8);
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
            emit ReceiverStatusUpdated("Client connected; saving to file");
            QDataStream fileWriteStream(&file);
            QByteArray fileBytes = ioSocket->readAll();
            std::string hex = fileBytes.toStdString();
            fileWriteStream.writeRawData(hex.c_str(), hex.length());
            fileSize -= hex.length();
            file.close();
        } else {
            emit ReceiverStatusUpdated(file.errorString());
        }
    }
}

/*------------------------------------------------------------------------------------------------------------------
-- FUNCTION SongSelected
--
-- DATE: April 13, 2018
--
-- DESIGNER: Alex Xia, Juliana French, Luke Lee, Vafa Saei
--
-- PROGRAMMER: Alex Xia, Juliana French
--
-- INTERFACE: void TransferModule::SongSelected(QString songName)
--                  songName : name of song to download from server
--
-- RETURNS: void
--
-- NOTES:
-- Call this function to send a valid filename to server to transfer back. 
-- The segment will be in the format "filenameMySong.mp3".
----------------------------------------------------------------------------------------------------------------------*/
void TransferModule::SongSelected(QString songName)
{
    fileToTransfer = songName;

    QString fileName = "filename";
    fileName += songName;
    QByteArray fileBytes = fileName.toUtf8();
    QDataStream socketWriteStream(ioSocket);
    socketWriteStream << fileBytes;
}

/*------------------------------------------------------------------------------------------------------------------
-- FUNCTION ServerReceivedBytes
--
-- DATE: April 13, 2018
--
-- DESIGNER: Alex Xia, Juliana French, Luke Lee, Vafa Saei
--
-- PROGRAMMER: Alex Xia, Juliana French
--
-- INTERFACE: void TransferModule::ServerReceivedBytes()
--
-- RETURNS: void
--
-- NOTES:
-- This function should not be called directly. It is triggered as a server when enough data arrives (max ~8188 bytes)
-- on its socket connected to a client. 
-- It will extract the first 8 bytes from the packet to see what is left on the socket to read:
--      filename - a file to sent back
--      filesize - sizeof a file in bytes to be sent back
-- For filename, it'll send a packet prefixed with "filesize" and containing size of file requested back to the client
-- For filesize, it'll parse the file size value, and if it was the same as the previously sent file size
-- will then read & send the file through the socket. 
----------------------------------------------------------------------------------------------------------------------*/
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
            emit SenderStatusUpdated("Connected, sending");
        } else {
            emit SenderStatusUpdated("Failed to open file " + fileToSend + ": " + file.errorString());
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
                emit SenderStatusUpdated("Connected & sending");
            } else {
               emit SenderStatusUpdated("Failed to open file " + nextFileToSend + ": " + file.errorString());
            }
        }
    }
}

/*------------------------------------------------------------------------------------------------------------------
-- FUNCTION ClientConnected
--
-- DATE: April 13, 2018
--
-- DESIGNER: Alex Xia, Juliana French, Luke Lee, Vafa Saei
--
-- PROGRAMMER: Alex Xia, Juliana French
--
-- INTERFACE: void TransferModule::HandleConnect()
--
-- RETURNS: void
--
-- NOTES:
-- This function is triggered as a server once a connect connects to it. It should not be called directly.
-- The client is then sent a list of songs prefixed with "filelist".
----------------------------------------------------------------------------------------------------------------------*/
void TransferModule::ClientConnected()
{
    ioSocket = receiver->nextPendingConnection();
    emit ReceiverStatusUpdated("A new client connected");
    connect(ioSocket, &QTcpSocket::disconnected, this, &TransferModule::HandleDisconnect);
    connect(ioSocket, &QTcpSocket::readyRead, this, &TransferModule::ServerReceivedBytes);
    QString fileNames = "filelist";
    fileNames += playlistToSend;
    QByteArray fileBytes = fileNames.toUtf8();
    QDataStream socketWriteStream(ioSocket);
    socketWriteStream << fileBytes;
}
