#include "mediaplayermodule.h"

MediaPlayerModule::MediaPlayerModule(QObject *parent) : QObject(parent)
{
    player = new QMediaPlayer;
    player->setVolume(100); //haha
}

MediaPlayerModule::~MediaPlayerModule()
{
    delete player;
}

void MediaPlayerModule::Play()
{
    if (player->state() == QMediaPlayer::StoppedState)
    {
        QString fileName = settings->GetFileName();
        QFile fileReadCheck(fileName);
        if (fileReadCheck.open(QIODevice::ReadOnly))
        {
            fileReadCheck.close();
            player->setMedia(QUrl::fromLocalFile(fileName));
            player->setPlaybackRate(1);
        }
        // file unopenable, assume not found
        else
        {
            QMessageBox popup;
            popup.setText("File not found in local directory:\n" + fileName);
            popup.exec();
        }
    }
    player->play();
}

void MediaPlayerModule::Pause()
{
    player->pause();
}

void MediaPlayerModule::Stop()
{
    player->stop();
}

void MediaPlayerModule::FastForward()
{
    qreal currentSpeed = player->playbackRate();
    player->setPlaybackRate(currentSpeed + 0.1);
}

void MediaPlayerModule::SlowForward()
{
    qreal currentSpeed = player->playbackRate();
    player->setPlaybackRate(currentSpeed - 0.1);
}

void MediaPlayerModule::ChangeSongPosition(int seconds)
{
    player->setPosition(seconds * 1000);
}
