#include "iosocketpair.h"

IOSocketPair::IOSocketPair(QObject *parent, QAudioFormat* format) : QObject(parent)
{
    input = new QAudioInput(*format, this);
    output = new QAudioOutput(*format, this);
    output->setVolume(1.0);

    recvSocket = nullptr;

    sendSocket = new QTcpSocket;
    sendStream = new QDataStream(sendSocket);
    //cant connect here, funky include error
}

IOSocketPair::~IOSocketPair()
{
    input->stop();
    output->stop();
    if (sendSocket != nullptr)
    {
        sendSocket->readAll();
        sendSocket->disconnect();
        sendSocket->disconnectFromHost();
        sendSocket->deleteLater();
        sendSocket = nullptr;
    }
    if (recvSocket != nullptr)
    {
        recvSocket->readAll();
        recvSocket->deleteLater();
    }
    if (sendStream != nullptr)
        delete sendStream;
    output->deleteLater();
    input->deleteLater();
    recvSocket->deleteLater();
}
