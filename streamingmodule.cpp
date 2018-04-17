#include "streamingmodule.h"
#define CLIENT_PORT 7000 //clients will host on this
#define SERVER_PORT 8000 //server will host on this

/*------------------------------------------------------------------------------------------------------------------
-- SOURCE FILE: streamingmodule.cpp - Handles all streaming functions, for both microphone & files
--
-- PROGRAM: CommAudio Networked Media Player
--
-- DATE: April 7, 2018
--
-- DESIGNER: Alex Xia, Juliana French, Luke Lee, Vafa Saei
--
-- PROGRAMMER: Alex Xia, Juliana French, Luke Lee
--
-- FUNCTIONS:
    StreamingModule(QObject* parent = nullptr);
    ~StreamingModule();
    void AttemptStreamConnect();
    void AttemptStreamDisconnect();
    void StartReceiver();
    void RemoveSocketPair(QHostAddress);
    void ClientConnected();
    void ClientDisconnected();
    void ServerDisconnected();
    void StartAudioOutput();
    void StartAudioInput();
    void GetSocketError(QTcpSocket::SocketError);
--
-- NOTES:
-- This class handles all streaming(non-saving) functionalities of the media player. This include the microphone,
-- file streaming, and multicasting features. For the most part, it is an isolated module, connected only to
-- MainWindow by Qt signals; it connects on different ports than TransferModule and runs on a separate thread. 
-- Like the other 2 modules(StreamingModule & MediaPlayerModule), it shares a pointer to a 
-- SettingsWindow object for getting user input/settings; 
-- 
-- After entering this mode by selecting streaming or microphone under settings
-- The users on the client & server sides both need to start their Speakers(receivers).
-- Then the client will need to click Start mic, and it'll do the following:
-- (better illustrated in the sequence diagram)
-- 
-- Client                                 Server
--   |                                      |    
-- Start receiver                   Start receiver 
-- on port 7000                     on port 8000
-- Listen                                Listen
--   |                                      |    
-- Connect to given ----------------------> |
-- ip, port 8000                    Extract client ip
--   |    <------------------------ Connect to extracted 
--   |                              ip, port 7000
--   |                                      |  
-- Pair formed                      Pair formed
--   |                                      |  
-- Send mic buffer ---------------> Recv to speaker buffer
-- Recv to speaker buffer <-------- Send mic buffer
-- 
-- ^Above is 1 connection, the server may have multiple connections for multiple clients
-- File streaming is mostly same as microphone, but the mic on server is replaced with a file buffer.
-- The speaker on the server is disabled, as well as the the mic on the clients.
-- As soon as a client connects, audio data from the same file is sent to it.
--
-- Multicasting is almost the same as File streaming, with the one exception being that clients aren't fed
-- audio data until a button on MainWindow is clicked. Then it loops through the list of connection clients
-- and feed each one audio data from the same file.
--
-- To ensure each socket is transferring at 100% capacity, the sockets are 
-- send/receive only.
-- A pair of QTcpServers & QTcpSockets (1 server + 1 socket per host) is used per connection, eg.
--      Client                           Server
-- 1x listening server              1x listening server
--              for each connection:
-- 1x sendSocket -----------------> 1x recvSocket
-- 1x recvSocket <----------------- 1x sendSocket
-- 2 sockets used by each host, 4 sockets used per client-server pair
-- Each pair of sendSocket/recvSocket on the same host is saved as an IOSocketPair object,
-- and added as a value to a map of connections with the ip of the other side as the key. 
----------------------------------------------------------------------------------------------------------------------*/
StreamingModule::StreamingModule(QObject *parent) :
    QObject(parent)
{
    format = new QAudioFormat();
    format->setSampleRate(96000);
    format->setChannelCount(1);
    format->setSampleSize(16);
    format->setCodec("audio/pcm");
    format->setByteOrder(QAudioFormat::LittleEndian);
    format->setSampleType(QAudioFormat::UnSignedInt);

    receiver = new QTcpServer(this);
    connect(receiver, &QTcpServer::newConnection, this, &StreamingModule::ClientConnected);
}

StreamingModule::~StreamingModule()
{
    delete receiver;
    delete format;
}

/*------------------------------------------------------------------------------------------------------------------
-- FUNCTION StartReceiver
--
-- DATE: April 11, 2018
--
-- DESIGNER: Alex Xia, Juliana French, Luke Lee, Vafa Saei
--
-- PROGRAMMER: Alex Xia, Luke Lee
--
-- INTERFACE: void StreamingModule::StartReceiver()
--
-- RETURNS: void
--
-- NOTES:
-- Call this function to let a QTcpServer(receiver) start listening on the host if one isn't already started
-- Starts the receiver for both client & server, but with client hosting on 7000 and server hosting on 8000.
-- If running as server in streaming mode, also checks that a .wav file is being used to stream.
-- If a .wav file is not being used to stream, then receiver is not started.  
----------------------------------------------------------------------------------------------------------------------*/
void StreamingModule::StartReceiver()
{
    if (receiver->isListening())
        return;
    //needs to make sure only .wav is used when streaming
    if ((settings->GetTransferMode() == "streaming" || settings->GetTransferMode() == "multicast") && settings->GetHostMode() == "Server")
    {
        QStringRef fileType = settings->GetFileName().rightRef(4);
        if (fileType != ".wav")
        {
            emit WrongFileType();
            return;
        }
    }
    int port = SERVER_PORT;
    if (settings->GetHostMode() == "Client")
        port = CLIENT_PORT;
    if(!receiver->listen(QHostAddress::Any, port))
    {
        emit ReceiverStatusUpdated("Error: " + receiver->errorString());
    }
    else
    {
        emit ReceiverStatusUpdated("Running");
        emit ReceiverReady(true);
    }
}

/*------------------------------------------------------------------------------------------------------------------
-- FUNCTION AttemptStreamConnect
--
-- DATE: April 11, 2018
--
-- DESIGNER: Alex Xia, Juliana French, Luke Lee, Vafa Saei
--
-- PROGRAMMER: Alex Xia
--
-- INTERFACE: void StreamingModule::AttemptStreamConnect()
--
-- RETURNS: void
--
-- NOTES:
-- Call this function as a client to connect in microphone or streaming mode.
-- Simply tries to connect to Ip address specified in settings window on port 8000.
----------------------------------------------------------------------------------------------------------------------*/
void StreamingModule::AttemptStreamConnect()
{
    if(settings->GetHostMode() == "Client")
    {
        IOSocketPair* newPair = new IOSocketPair(this, format);
        QHostAddress destnAddress(settings->GetIpAddress());
        connectionList.insert(destnAddress, newPair);

        connect(newPair->sendSocket, &QTcpSocket::connected, this, &StreamingModule::StartAudioInput);
        connect(newPair->sendSocket, &QTcpSocket::disconnected, this, &StreamingModule::ServerDisconnected);
        connect(newPair->sendSocket, QOverload<QTcpSocket::SocketError>::of(&QTcpSocket::error), this, &StreamingModule::GetSocketError);
        newPair->sendSocket->connectToHost(destnAddress, SERVER_PORT);
    }
    //shouldn't call this as server
}

/*------------------------------------------------------------------------------------------------------------------
-- FUNCTION GetSocketError
--
-- DATE: April 11, 2018
--
-- DESIGNER: Alex Xia, Juliana French, Luke Lee, Vafa Saei
--
-- PROGRAMMER: Alex Xia
--
-- INTERFACE: void StreamingModule::GetSocketError(QTcpSocket::SocketError socketError)
--                  socketError : error that occurred in the calling socket
--
-- RETURNS: void
--
-- NOTES:
-- This function signals MainWindow to display an error that occured in a socket.
-- It is triggered on error, so it should not be called directly.
----------------------------------------------------------------------------------------------------------------------*/
void StreamingModule::GetSocketError(QTcpSocket::SocketError socketError)
{
    QTcpSocket* sendSocket = (QTcpSocket*)sender();
    emit SenderStatusUpdated("Error: " + sendSocket->errorString());
}

/*------------------------------------------------------------------------------------------------------------------
-- FUNCTION AttemptStreamDisconnect
--
-- DATE: April 11, 2018
--
-- DESIGNER: Alex Xia, Juliana French, Luke Lee, Vafa Saei
--
-- PROGRAMMER: Alex Xia, Juliana French
--
-- INTERFACE: void StreamingModule::AttemptStreamDisconnect() 
--
-- RETURNS: void
--
-- NOTES:
-- Call this function to disconnect from microphone or streaming mode.
-- Clears the list of IOSocketPairs created when the connection was formed, and signals
-- MainWindow to enable UI elements to allow user to connect again.
----------------------------------------------------------------------------------------------------------------------*/
void StreamingModule::AttemptStreamDisconnect()
{
    if (AlreadyDisconnecting)
        return;
    AlreadyDisconnecting = true;
    //client has only 1 item, but code reusable for both modes
    for(auto it = connectionList.begin(); it != connectionList.end();)
    {
        IOSocketPair* clientPair = it.value();
        clientPair->deleteLater();
        it = connectionList.erase(it);
    }
    receiver->close();

    emit SenderStatusUpdated("Disconnected");
    emit ReceiverStatusUpdated("Disconnected");
    emit ReceiverReady(false);
    AlreadyDisconnecting = false;
}

/*------------------------------------------------------------------------------------------------------------------
-- FUNCTION ClientDisconnected
--
-- DATE: April 11, 2018
--
-- DESIGNER: Alex Xia, Juliana French, Luke Lee, Vafa Saei
--
-- PROGRAMMER: Alex Xia
--
-- INTERFACE: void StreamingModule::ClientDisconnected()
--
-- RETURNS: void
--
-- NOTES:
-- This function is triggered as a server when a client disconnected from it. It should not be called directly.
-- Removes the IOSocketPair that contained the connection's sockets from the map of connections.
----------------------------------------------------------------------------------------------------------------------*/
void StreamingModule::ClientDisconnected()
{
    emit ReceiverStatusUpdated("A client disconnected");
    QTcpSocket* recvSocket = (QTcpSocket*)sender();
    RemoveSocketPair(recvSocket->peerAddress());
}

/*------------------------------------------------------------------------------------------------------------------
-- FUNCTION ClientConnected
--
-- DATE: April 11, 2018
--
-- DESIGNER: Alex Xia, Juliana French, Luke Lee, Vafa Saei
--
-- PROGRAMMER: Alex Xia
--
-- INTERFACE: void StreamingModule::ClientConnected()
--
-- RETURNS: void
--
-- NOTES:
-- This function is triggered as a server when a client connected to it. It should not be called directly.
-- Adds a new IOSocketPair that contained the connection's sockets to the map of connections.
----------------------------------------------------------------------------------------------------------------------*/
void StreamingModule::ClientConnected()
{
    QTcpSocket* recvSocket = receiver->nextPendingConnection();
    connect(recvSocket, &QTcpSocket::disconnected, this, &StreamingModule::ClientDisconnected);
    connect(recvSocket, &QTcpSocket::readyRead, this, &StreamingModule::StartAudioOutput);
    emit ReceiverStatusUpdated("A client connected");
    if (settings->GetHostMode() == "Client")
    {
        QMap<QHostAddress, IOSocketPair*>::iterator nonConstIt = connectionList.find(recvSocket->peerAddress());
        IOSocketPair* nonConstPair = nonConstIt.value();
        nonConstPair->recvSocket = recvSocket;
    }
    if (settings->GetHostMode() == "Server")
    {
        IOSocketPair* newPair = new IOSocketPair(this, format);
        newPair->recvSocket = recvSocket;
        connectionList.insert(recvSocket->peerAddress(), newPair);

        connect(newPair->sendSocket, &QTcpSocket::connected, this, &StreamingModule::StartAudioInput);
        connect(newPair->sendSocket, &QTcpSocket::disconnected, this, &StreamingModule::ServerDisconnected);
        connect(newPair->sendSocket, QOverload<QTcpSocket::SocketError>::of(&QTcpSocket::error), this, &StreamingModule::GetSocketError);
        newPair->sendSocket->connectToHost(recvSocket->peerAddress(), CLIENT_PORT);
        emit SenderStatusUpdated("Now connecting to new client");
    }
}

/*------------------------------------------------------------------------------------------------------------------
-- FUNCTION StartAudioOutput
--
-- DATE: April 11, 2018
--
-- DESIGNER: Alex Xia, Juliana French, Luke Lee, Vafa Saei
--
-- PROGRAMMER: Alex Xia
--
-- INTERFACE: void StreamingModule::StartAudioOutput()
--
-- RETURNS: void
--
-- NOTES:
-- This function is triggered when a socket has audio data ready to be pushed to speaker. It should not be called directly.
-- Finds the corresponding IOSocketPair in the map of connections by the signaling socket's ip address, and pushes its
-- data to current host' speaker. 
----------------------------------------------------------------------------------------------------------------------*/
void StreamingModule::StartAudioOutput()
{
    QTcpSocket* recvSocket = (QTcpSocket*)sender();
    IOSocketPair* pair = connectionList.value(recvSocket->peerAddress(), nullptr);
    emit ReceiverStatusUpdated("Received audio, speaker playing");
    if (pair == nullptr)
    {
        qDebug() << "pair null now";
        return;
    }
    if (pair->output->state() == QAudio::IdleState || pair->output->state() == QAudio::StoppedState)
        pair->output->start(recvSocket); //output is a speaker and always will be
}

/*------------------------------------------------------------------------------------------------------------------
-- FUNCTION StartAudioInput
--
-- DATE: April 11, 2018
--
-- DESIGNER: Alex Xia, Juliana French, Luke Lee, Vafa Saei
--
-- PROGRAMMER: Alex Xia, Juliana French
--
-- INTERFACE: void StreamingModule::StartAudioInput()
--
-- RETURNS: void
--
-- NOTES:
-- This function is triggered when a socket on either the client or server side has connected to the other side. 
-- It should not be called directly.
-- Finds the corresponding IOSocketPair in the map of connections by the signaling socket's ip address.
-- If the host is in microphone mode, the corresponding IOSocketPair's microphone interface is passed (and start being read)
-- to its sendSocket. 
-- If the host is in streaming mode, a file is read & passed as a stream to the sendSocket instead. 
----------------------------------------------------------------------------------------------------------------------*/
void StreamingModule::StartAudioInput()
{
    QTcpSocket* sendSocket = (QTcpSocket*)sender();
    IOSocketPair* pair = connectionList.value(sendSocket->peerAddress());

    if (settings->GetTransferMode() == "microphone")
    {
        pair->input->start(sendSocket); //input is mic
        emit SenderStatusUpdated("Connected, say something to chat");
    }
    if (settings->GetTransferMode() == "streaming")
    {
        if(settings->GetHostMode() == "Server")
        {
            QString filename = settings->GetFileName();
            QFile fileToStream(filename);
            if(fileToStream.open(QIODevice::ReadOnly))
            {
                QByteArray fileBytes = fileToStream.readAll();
                *(pair->sendStream) << fileBytes;
                fileToStream.close();
                emit SenderStatusUpdated("Connected; sending");
            } else {
                emit SenderStatusUpdated(fileToStream.errorString());
            }
        }
        if (settings->GetHostMode()== "Client")
        {
            emit SenderStatusUpdated("Connected.");
        }
    }
    if (settings->GetTransferMode() == "multicast" && settings->GetHostMode() == "Client")
    {
        emit SenderStatusUpdated("Connected.");
    }
}

/*------------------------------------------------------------------------------------------------------------------
-- FUNCTION MulticastAudioInput
--
-- DATE: April 11, 2018
--
-- DESIGNER: Alex Xia, Juliana French, Luke Lee, Vafa Saei
--
-- PROGRAMMER: Juliana French, Alex Xia
--
-- INTERFACE: void StreamingModule::MulticastAudioInput()
--
-- RETURNS: void
--
-- NOTES:
-- This function is triggered when a Server running in multicast mode clicks a button to start multicasting to all
-- connected clients. As the entire module runs on a QThread, it should not be called directly.
-- If the file selected for streaming via multicast cannot be opened, a signal is emitted to MainWindow
-- to display an error message popup. Otherwise it loops through the list of clients, and sends the file to each one
-- via a datastream.
----------------------------------------------------------------------------------------------------------------------*/
void StreamingModule::MulticastAudioInput()
{
    if(settings->GetHostMode() == "Server")
    {
        QString filename = settings->GetFileName();
        QFile fileToStream(filename);
        if (fileToStream.open(QIODevice::ReadOnly))
        {
            emit SenderStatusUpdated("Connected; sending");
            fileToStream.close();
        }
        else
        {
            emit SenderStatusUpdated(fileToStream.errorString());
            return;
        }

        for (auto it = connectionList.begin(); it != connectionList.end(); ++it)
        {
            IOSocketPair* clientPair = it.value();
            fileToStream.open(QIODevice::ReadOnly);
            QByteArray fileBytes = fileToStream.readAll();
            *(clientPair->sendStream) << fileBytes;
            fileToStream.close();
        }
    }
}

/*------------------------------------------------------------------------------------------------------------------
-- FUNCTION RemoveSocketPair
--
-- DATE: April 11, 2018
--
-- DESIGNER: Alex Xia, Juliana French, Luke Lee, Vafa Saei
--
-- PROGRAMMER: Alex Xia
--
-- INTERFACE: void StreamingModule::RemoveSocketPair(QHostAddress destnIp)
--              destnIp : ip of the IOSocketPair to remove, same as ip of other side
--
-- RETURNS: void
--
-- NOTES:
-- Call this function to remove an IOSocketPair from connection map based on ip.
-- This should be called as server when a client disconnected.
----------------------------------------------------------------------------------------------------------------------*/
void StreamingModule::RemoveSocketPair(QHostAddress destnIp)
{
    //this is const
    const IOSocketPair* pairToRemove = connectionList.value(destnIp);
    delete pairToRemove;
    connectionList.remove(destnIp);
}

/*------------------------------------------------------------------------------------------------------------------
-- FUNCTION ServerDisconnected
--
-- DATE: April 11, 2018
--
-- DESIGNER: Alex Xia, Juliana French, Luke Lee, Vafa Saei
--
-- PROGRAMMER: Alex Xia
--
-- INTERFACE: void StreamingModule::ServerDisconnected()
--
-- RETURNS: void
--
-- NOTES:
-- This function is triggered when a sendSocket disconnected, so MainWindow is signaled to display a status.
-- It should not be called directly. If triggered as client, streaming should end abruptly.
----------------------------------------------------------------------------------------------------------------------*/
void StreamingModule::ServerDisconnected()
{
    emit ReceiverStatusUpdated("Server disconnected");
}

/*------------------------------------------------------------------------------------------------------------------
-- FUNCTION operator<
--
-- DATE: April 11, 2018
--
-- DESIGNER: Alex Xia
--
-- PROGRAMMER: Alex Xia
--
-- INTERFACE: bool operator<(const QHostAddress l, const QHostAddress r)
--
-- RETURNS: void
--
-- NOTES:
-- This operator allows QHostAddresses to be used as keys in a map of IOSocketPairs; it is essentially used by Qt 
-- as the comparator function for the map of connections.
-- It should never be needed or called directly.
----------------------------------------------------------------------------------------------------------------------*/
bool operator<(const QHostAddress l, const QHostAddress r)
{
    return l.toIPv4Address() < r.toIPv4Address();
}
