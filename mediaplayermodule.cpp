#include "mediaplayermodule.h"
/******************************************************************************
 * SOURCE FILE:         mediaplayermodule.cpp
 *
 * PROGRAM:             CommAudio
 *
 * FUNCTIONS:           public MediaPlayerModule(QObject *parent)
 *                      public ~MediaPlayerModule()
 *                      public void Play()
 *                      void Pause()
 *                      void Stop()
 *                      void FastForward()
 *                      void SlowForward()
 *                      void ChangeSongPosition(int)
 *
 * DATE:
 *
 * REVISIONS:
 *
 * DESIGNER:
 *
 * PROGRAMMER:
 *
 * NOTES:
 *
 ******************************************************************************/

/******************************************************************************
 * FUNCTION:
 *
 * DATE:
 *
 * REVISIONS:
 *
 * DESIGNER:
 *
 * PROGRAMMER:
 *
 * INTERFACE:
 *
 * RETURNS:
 *
 * NOTES:
 *
 ******************************************************************************/
MediaPlayerModule::MediaPlayerModule(QObject *parent) : QObject(parent)
{
    player = new QMediaPlayer;
    player->setVolume(100); //haha
}

/******************************************************************************
 * FUNCTION:
 *
 * DATE:
 *
 * REVISIONS:
 *
 * DESIGNER:
 *
 * PROGRAMMER:
 *
 * INTERFACE:
 *
 * RETURNS:
 *
 * NOTES:
 *
 ******************************************************************************/
MediaPlayerModule::~MediaPlayerModule()
{
    delete player;
}

/******************************************************************************
 * FUNCTION:
 *
 * DATE:
 *
 * REVISIONS:
 *
 * DESIGNER:
 *
 * PROGRAMMER:
 *
 * INTERFACE:
 *
 * RETURNS:
 *
 * NOTES:
 *
 ******************************************************************************/
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

/******************************************************************************
 * FUNCTION:
 *
 * DATE:
 *
 * REVISIONS:
 *
 * DESIGNER:
 *
 * PROGRAMMER:
 *
 * INTERFACE:
 *
 * RETURNS:
 *
 * NOTES:
 *
 ******************************************************************************/
void MediaPlayerModule::Pause()
{
    player->pause();
}

/******************************************************************************
 * FUNCTION:
 *
 * DATE:
 *
 * REVISIONS:
 *
 * DESIGNER:
 *
 * PROGRAMMER:
 *
 * INTERFACE:
 *
 * RETURNS:
 *
 * NOTES:
 *
 ******************************************************************************/
void MediaPlayerModule::Stop()
{
    player->stop();
}

/******************************************************************************
 * FUNCTION:
 *
 * DATE:
 *
 * REVISIONS:
 *
 * DESIGNER:
 *
 * PROGRAMMER:
 *
 * INTERFACE:
 *
 * RETURNS:
 *
 * NOTES:
 *
 ******************************************************************************/
void MediaPlayerModule::FastForward()
{
    qreal currentSpeed = player->playbackRate();
    player->setPlaybackRate(currentSpeed + 0.1);
}

/******************************************************************************
 * FUNCTION:
 *
 * DATE:
 *
 * REVISIONS:
 *
 * DESIGNER:
 *
 * PROGRAMMER:
 *
 * INTERFACE:
 *
 * RETURNS:
 *
 * NOTES:
 *
 ******************************************************************************/
void MediaPlayerModule::SlowForward()
{
    qreal currentSpeed = player->playbackRate();
    player->setPlaybackRate(currentSpeed - 0.1);
}

/******************************************************************************
 * FUNCTION:
 *
 * DATE:
 *
 * REVISIONS:
 *
 * DESIGNER:
 *
 * PROGRAMMER:
 *
 * INTERFACE:
 *
 * RETURNS:
 *
 * NOTES:
 *
 ******************************************************************************/
void MediaPlayerModule::ChangeSongPosition(int seconds)
{
    player->setPosition(seconds * 1000);
}
