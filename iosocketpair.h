#ifndef IOSOCKETPAIR_H
#define IOSOCKETPAIR_H

#include <QObject>
#include <QTcpSocket>
#include <QAudioFormat>
#include <QAudioInput>
#include <QAudioOutput>
#include <QDebug>
#include <QFile>
#include <QDataStream>

class IOSocketPair : public QObject
{
    Q_OBJECT
public:
    explicit IOSocketPair(QObject* parent = nullptr, QAudioFormat* format = nullptr);
    ~IOSocketPair();

    QDataStream* sendStream;
    QAudioOutput* output;
    QAudioInput* input;
    QTcpSocket* sendSocket;
    QTcpSocket* recvSocket = nullptr;
};

#endif // IOSOCKETPAIR_H
