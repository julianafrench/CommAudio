#ifndef MEDIAPLAYERMODULE_H
#define MEDIAPLAYERMODULE_H

#include <QObject>
#include <QMediaPlayer>
#include <QMediaContent>
#include <QFileDialog>
#include <QMessageBox>
#include <QFile>
#include "settingswindow.h"

class MediaPlayerModule : public QObject
{
    Q_OBJECT
public:
    explicit MediaPlayerModule(QObject *parent = nullptr);
    ~MediaPlayerModule();
    QMediaPlayer* player;
    SettingsWindow* settings;

signals:

public slots:
    void Play();
    void Pause();
    void Stop();
    void FastForward();
    void SlowForward();
    void ChangeSongPosition(int);
};

#endif // MEDIAPLAYERMODULE_H
