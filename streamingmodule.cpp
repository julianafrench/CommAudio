#include "streamingmodule.h"
#define DEF_PORT 7000

StreamingModule::StreamingModule(QObject *parent, SettingsWindow* settings) : QObject(parent)
{
    format = new QAudioFormat();
    format->setSampleRate(96000); //<--acceptable
    format->setChannelCount(1);
    format->setSampleSize(32);
    format->setCodec("audio/pcm");
    format->setByteOrder(QAudioFormat::LittleEndian);
    format->setSampleType(QAudioFormat::UnSignedInt);

    input = new QAudioInput(*format, this);
    output = new QAudioOutput(*format, this);
    output->setVolume(1.0);

    receiver = new QTcpServer(this);
    connect(receiver, &QTcpServer::newConnection, this, &StreamingModule::ClientConnected);

    sendSocket = new QTcpSocket;
    connect(sendSocket, &QTcpSocket::connected, this, &StreamingModule::StartAudioInput);
    connect(sendSocket, &QTcpSocket::disconnected, this, &StreamingModule::AttemptStreamDisconnect);
    connect(sendSocket, QOverload<QTcpSocket::SocketError>::of(&QTcpSocket::error), this, &StreamingModule::GetSocketError);

    recvSocket = Q_NULLPTR;
    sendStream = new QDataStream(sendSocket);

    this->settings = settings;
}

StreamingModule::~StreamingModule()
{
    delete input;
    delete output;
    if (recvSocket != Q_NULLPTR && recvSocket->isOpen())
        delete recvSocket;
    delete sendSocket;
    delete sendStream;
    delete receiver;
}

void StreamingModule::StartReceiver()
{
    if (receiver->isListening())
        return;
    int tempPort = DEF_PORT;
    if (settings->GetHostMode() == "Client")
        tempPort = 8000;
    if(!receiver->listen(QHostAddress::Any, tempPort))
    {
        emit ReceiverStatusUpdated("Error: " + receiver->errorString());
    }
    else
    {
        emit ReceiverStatusUpdated("Receiver: running");
        emit ReceiverReady(true);
    }
}

void StreamingModule::AttemptStreamConnect()
{
    if(settings->GetHostMode() == "Client")
    {
        QString ip = settings->GetIpAddress();
        sendSocket->connectToHost(ip, DEF_PORT);
//        if(!sendSocket->waitForConnected(5000)) <- this blocks, too much trouble for just a status msg
//        {
//            emit SenderStatusUpdated("Error: " + sendSocket->errorString());
//        }
    }
    //shouldn't call this as server
}

//feel free to try & extract errorString from this slot param, cause i cant
void StreamingModule::GetSocketError(QTcpSocket::SocketError socketError)
{
    emit SenderStatusUpdated("Error: " + sendSocket->errorString());
}

void StreamingModule::AttemptStreamDisconnect()
{
    receiver->close();
    sendSocket->disconnectFromHost();
    output->stop();

    emit SenderStatusUpdated("Sender: disconnected");
    emit ReceiverStatusUpdated("Receiver: disconnected");
    emit ReceiverReady(false);
}

void StreamingModule::ClientDisconnected()
{
    emit ReceiverStatusUpdated("Receiver: a client disconnected, so i must die too");
    AttemptStreamDisconnect();
}

void StreamingModule::ClientConnected()
{
    recvSocket = receiver->nextPendingConnection();
    //this connect call cant go in constrc, cant call connect on null QObject
    connect(recvSocket, &QTcpSocket::disconnected, this, &StreamingModule::ClientDisconnected);

    emit ReceiverStatusUpdated("Receiver: a sender connected");
    if (settings->GetHostMode() == "Client")
    {
        connect(recvSocket, &QTcpSocket::readyRead, this, &StreamingModule::StartAudioOutput);
    }
    //server must extract client ip & connect to it to send stuff back
    if (settings->GetHostMode() == "Server")
    {
        QHostAddress clientAddress = recvSocket->peerAddress();
        int tempPort = 8000;
        sendSocket->connectToHost(clientAddress, tempPort);
    }
}

void StreamingModule::StartAudioOutput()
{
    if (output->state() == QAudio::IdleState || output->state() == QAudio::StoppedState)
        output->start(recvSocket); //output is a speaker and always will be
}

void StreamingModule::StartAudioInput()
{
    if (settings->GetTransferMode() == "microphone")
    {
        input->start(sendSocket); //input is mic
        emit SenderStatusUpdated("Sender: connected, start talkin");
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
                *sendStream << fileBytes;
                fileToStream.close();
                emit SenderStatusUpdated("Sender: connected & sending");
            } else {
                emit SenderStatusUpdated("Sender: failed to open file " + filename + ": " + fileToStream.errorString());
            }
        }
        if (settings->GetHostMode()== "Client")
        {
            emit SenderStatusUpdated("Sender: connected, doesnt send");
        }
    }
    // file doesnt do anything yet for client
}
