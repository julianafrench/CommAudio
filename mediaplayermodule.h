#ifndef MEDIAPLAYERMODULE_H
#define MEDIAPLAYERMODULE_H

#include <QObject>
#include <QMediaPlayer>
#include <QMediaContent>
#include <QFileDialog>
#include <QMessageBox>

class MediaPlayerModule : public QObject
{
    Q_OBJECT
public:
    explicit MediaPlayerModule(QObject *parent = nullptr);
    ~MediaPlayerModule();
    QMediaPlayer* player;

signals:

public slots:
    void ShowFilePicker();
    void Play();
    void Pause();
    void Stop();
    void FastForward();
    void SlowForward();
    void ChangeSongPosition(int);
private:
    QString fileName = "";
};

#endif // MEDIAPLAYERMODULE_H
