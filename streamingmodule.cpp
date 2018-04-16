#include "streamingmodule.h"
#define CLIENT_PORT 7000 //clients will host on this
#define SERVER_PORT 8000 //server will host on this

StreamingModule::StreamingModule(QObject *parent) :
    QObject(parent)
{
    format = new QAudioFormat();
    format->setSampleRate(96000); //96000 old
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

void StreamingModule::StartReceiver()
{
    if (receiver->isListening())
        return;
    //needs to make sure only .wav is used when streaming
    if (settings->GetTransferMode() == "streaming" && settings->GetHostMode() == "Server")
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

//feel free to try & extract errorString from this slot param, cause i cant
void StreamingModule::GetSocketError(QTcpSocket::SocketError socketError)
{
    QTcpSocket* sendSocket = (QTcpSocket*)sender();
    emit SenderStatusUpdated("Error: " + sendSocket->errorString());
}

void StreamingModule::AttemptStreamDisconnect()
{
    if (AlreadyDisconnecting)
        return;
    AlreadyDisconnecting = true;
    //client has only 1 item, but code reusable for both modes
    for(auto it = connectionList.begin(); it != connectionList.end();)
    {
        IOSocketPair* clientPair = it.value();
        //clientPair->output->stop();
        delete clientPair;
        it = connectionList.erase(it);
    }
    receiver->close();

    emit SenderStatusUpdated("Disconnected");
    emit ReceiverStatusUpdated("Disconnected");
    emit ReceiverReady(false);
    AlreadyDisconnecting = false;
}

void StreamingModule::ClientDisconnected()
{
    emit ReceiverStatusUpdated("A client disconnected");
    QTcpSocket* recvSocket = (QTcpSocket*)sender();
    RemoveSocketPair(recvSocket->peerAddress());
}

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

void StreamingModule::StartAudioOutput()
{
    QTcpSocket* recvSocket = (QTcpSocket*)sender();
    IOSocketPair* pair = connectionList.value(recvSocket->peerAddress(), nullptr);
    //since pair is pointer, should be updated now
    if (pair->output->state() == QAudio::IdleState || pair->output->state() == QAudio::StoppedState)
        pair->output->start(recvSocket); //output is a speaker and always will be
}

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
                emit SenderStatusUpdated("Failed to open file " + filename + ": " + fileToStream.errorString());
            }
        }
        if (settings->GetHostMode()== "Client")
        {
            emit SenderStatusUpdated("Connected, not sending yet");
        }
    }
    if (settings->GetTransferMode() == "multicast" && settings->GetHostMode() == "Client")
    {
        emit SenderStatusUpdated("Connected, not sending yet");
    }
}

void StreamingModule::MulticastAudioInput()
{
    if(settings->GetHostMode() == "Server")
    {
        QString filename = settings->GetFileName();
        QFile fileToStream(filename);
        if (fileToStream.open(QIODevice::ReadOnly))
        {
            emit SenderStatusUpdated("Connected; sending");
        }
        else
        {
            emit SenderStatusUpdated("Failed to open file " + filename + ": " + fileToStream.errorString());
            return;
        }

        for (auto it = connectionList.begin(); it != connectionList.end();)
        {
            IOSocketPair* clientPair = it.value();

            QByteArray fileBytes = fileToStream.readAll();
            *(clientPair->sendStream) << fileBytes;
        }
        fileToStream.close();
    }
}

void StreamingModule::RemoveSocketPair(QHostAddress destnIp)
{
    //this is const
    const IOSocketPair* pairToRemove = connectionList.value(destnIp);
    delete pairToRemove;
    connectionList.remove(destnIp);
}

void StreamingModule::ServerDisconnected()
{
    emit ReceiverStatusUpdated("Server disconnected");
}

bool operator<(const QHostAddress l, const QHostAddress r)
{
    return l.toIPv4Address() < r.toIPv4Address();
}
