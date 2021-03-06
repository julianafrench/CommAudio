#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#define SERVER  0
#define CLIENT  1
#define UNDEFINED -1

#include <QMainWindow>
#include <QThread>
#include <winsock2.h>   // winsock2.h needs to be include before windows.h, otherwise will have redefinition problem
#include <windows.h>

#include "settingswindow.h"
#include "streamingmodule.h"
#include "mediaplayermodule.h"
#include "transfermodule.h"

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();
    QString loadPlaylist();
    void displayPlaylistByRow(int row);
    void displayFileSizeByRow(int row);
    void clearPlaylist();

private slots:
    void on_actionClient_triggered();
    void on_actionServer_triggered();
    void on_actionConnect_triggered();
    void on_actionDisconnect_triggered();
    void on_actionSettings_triggered();
    void on_SaveButton_clicked();
    void EnableConnect();
    void EnableDisconnect();
    void UpdatePlaylist(QString);
    void UpdateSelectedFile(int, int);
    void ShowFilePicker();

    void UpdateSenderStatus(QString);
    void UpdateReceiverStatus(QString);
    void UpdateSettings();
    void AlertWrongFileType();

    void ToggleStreaming(bool);
    void InitializeSongDuration(qint64);
    void UpdateSongProgress(qint64);

private:
    Ui::MainWindow* ui;
    SettingsWindow* settings;
    QString playlist;
    int hostType;
    StreamingModule* streamer;
    QThread streamingThread;
    MediaPlayerModule* mediaPlayer;
    TransferModule* transferer;
    QStringList fileNames;
    QStringList fileSizes;
};

#endif // MAINWINDOW_H
