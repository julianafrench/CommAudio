#ifndef TRANSFERMODULE_H
#define TRANSFERMODULE_H

#include <QTcpSocket>
#include <QTcpServer>
#include <QDataStream>
#include <QDebug>
#include <QFile>
#include <QListWidget>
#include "settingswindow.h"

#define DEF_PORT 6000 //will not interfere with other modules

class TransferModule : public QObject
{
    Q_OBJECT
public:
    explicit TransferModule(QObject *parent = nullptr);
    ~TransferModule();
    void Connect(QString playlist = "");
    void Disconnect();
    SettingsWindow* settings;
    void SongSelected(QString);

signals:
    void Connected(); // for mainwindow use
    void Disconnected();
    void SenderStatusUpdated(QString);
    void ReceiverStatusUpdated(QString);
    void PlaylistReady(QString);

private:
    QTcpServer* receiver;
    QTcpSocket* ioSocket;
    QString playlistToSend; //server only
    QString fileToTransfer; //server & client

    void HandleConnect();
    void HandleDisconnect();
    void ClientReceivedBytes();
    void ServerReceivedBytes();
    void ClientConnected(); // (to server)

};

#endif // TRANSFERMODULE_H
