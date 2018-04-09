#ifndef STREAMINGMODULE_H
#define STREAMINGMODULE_H

#include <QObject>
#include <QTcpSocket>
#include <QTcpServer>
#include <QAudioFormat>
#include <QAudioInput>
#include <QAudioOutput>
#include <QDebug>
#include <QFile>
#include <QDataStream>

#include "settingswindow.h"

class StreamingModule : public QObject
{
    Q_OBJECT
public:
    StreamingModule(QObject* parent = nullptr, SettingsWindow* settings = nullptr);
    ~StreamingModule();

signals:
    void SenderStatusUpdated(const QString&);
    void ReceiverStatusUpdated(const QString&);
    void ReceiverReady(bool);

public slots:
    void AttemptStreamConnect();
    void AttemptStreamDisconnect();
    void StartReceiver();

private:
    QAudioOutput* output;
    QAudioInput* input;
    QAudioFormat* format;
    QTcpServer* receiver;
    QTcpSocket* recvSocket;
    QTcpSocket* sendSocket;
    QDataStream* sendStream;
    SettingsWindow* settings;

    void ToggleClientServerMode(const QString&);
    void ClientConnected();
    void ClientDisconnected();
    void StartAudioOutput();
    void StartAudioInput();
    void ConnectedToServer();
    void DisconnectedFromServer();
    void GetSocketError(QTcpSocket::SocketError);
};

#endif // STREAMINGMODULE_H
