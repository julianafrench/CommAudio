#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#define SERVER  0
#define CLIENT  1
#define UNDEFINED -1

#include <QMainWindow>
#include <winsock2.h>   // winsock2.h needs to be include before windows.h, otherwise will have redefinition problem
#include <windows.h>

#include "settingswindow.h"
#include "streamingmodule.h"

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
    QString getSelectedFile();

private slots:
    void on_actionExit_triggered();
    void on_actionServer_triggered();
    void on_actionClient_triggered();
    void on_actionConnect_triggered();
    void on_actionSettings_triggered();
    void on_actionHelp_triggered();
    void on_actionAbout_triggered();
    void on_saveBtn_clicked();

    void UpdateSettings();
    void ToggleClientServerUi();
    void ToggleStreaming(bool);
    void UpdateSenderStatus(QString);
    void UpdateReceiverStatus(QString);

private:
    Ui::MainWindow* ui;
    SettingsWindow* settings;
    QString playlist;
    int hostType;
    StreamingModule* streamer;
};

#endif // MAINWINDOW_H
