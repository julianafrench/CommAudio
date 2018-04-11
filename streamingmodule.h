#ifndef STREAMINGMODULE_H
#define STREAMINGMODULE_H

#include <QTcpServer>
#include "settingswindow.h"
#include "iosocketpair.h"

class StreamingModule : public QObject
{
    Q_OBJECT
public:
    StreamingModule(QObject* parent = nullptr);
    ~StreamingModule();
    SettingsWindow* settings;

signals:
    void SenderStatusUpdated(const QString&);
    void ReceiverStatusUpdated(const QString&);
    void ReceiverReady(bool);

public slots:
    void AttemptStreamConnect();
    void AttemptStreamDisconnect();
    void StartReceiver();

private:
    QAudioFormat* format;
    QTcpServer* receiver;
    QMap<QHostAddress, IOSocketPair*> connectionList;
    bool AlreadyDisconnecting = false;
    void RemoveSocketPair(QHostAddress);

private slots:
    void ClientConnected();
    void ClientDisconnected();
    void ServerDisconnected();

    void StartAudioOutput();
    void StartAudioInput();

    void GetSocketError(QTcpSocket::SocketError);

};

#endif // STREAMINGMODULE_H
