#ifndef SETTINGSWINDOW_H
#define SETTINGSWINDOW_H

#include <QWidget>
#include <QDialog>
#include <QFileDialog>

namespace Ui {
class SettingsWindow;
}

class SettingsWindow : public QDialog
{
    Q_OBJECT

public:
    explicit SettingsWindow(QWidget *parent = 0);
    ~SettingsWindow();
    Ui::SettingsWindow* ui;

    QString GetHostMode();
    QString GetTransferMode();
    QString GetIpAddress();
    QString GetBroadcastMode();
    QString GetFileName();
    void ShowFilePicker();

private:
    void ToggleClientServerUi(QString ignoreThis = "");

};

#endif // SETTINGSWINDOW_H
