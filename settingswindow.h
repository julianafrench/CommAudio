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

    QString GetHostMode();
    QString GetTransferMode();
    QString GetIpAddress();
    QString GetFileName();
    void SetFileName(QString);

private:
    void ToggleClientServerUi(QString ignoreThis = "");
    Ui::SettingsWindow* ui;
    QString selectedFileName = "";

};

#endif // SETTINGSWINDOW_H
