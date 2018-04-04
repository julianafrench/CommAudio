#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#define SERVER  0
#define CLIENT  1

#include <QMainWindow>
#include <winsock2.h>   // winsock2.h needs to be include before windows.h, otherwise will have redefinition problem
#include <windows.h>

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
    void on_saveBtn_clicked();

private:
    Ui::MainWindow *ui;
    QString playlist;
    int hostType;
};

#endif // MAINWINDOW_H
