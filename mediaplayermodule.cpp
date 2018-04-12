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

void MediaPlayerModule::ShowFilePicker()
{
    fileName = QFileDialog::getOpenFileName((QWidget*)this->parent(),
        tr("Choose a file to stream"), "", tr("(*.wav *.mp3)"));
}

void MediaPlayerModule::Play()
{
    if (player->state() == QMediaPlayer::StoppedState)
    {
        player->setMedia(QUrl::fromLocalFile(fileName));
        player->setPlaybackRate(1);
    }
    if (fileName == "")
    {
        QMessageBox msgBox;
        msgBox.setText("Please select a file first.");
        msgBox.exec();
        return;
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
