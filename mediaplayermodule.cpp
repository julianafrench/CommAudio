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
 * DATE:                April 16 2018
 *
 *
 * DESIGNER:            Alex Xia, Juliana French, Luke Lee, Vafa Dehghan Saei
 *
 * PROGRAMMER:          Alex Xia, Juliana French, Luke Lee, Vafa Dehghan Saei
 *
 * NOTES:               This file has all the controls for the media player related buttons.
 *                      These include Play, stop, pause, etc.
 *
 ******************************************************************************/

/******************************************************************************
 * FUNCTION:            MediaPlayerModule
 *
 * DATE:                April 16 2018
 *
 *
 * DESIGNER:            Alex Xia
 *
 * PROGRAMMER:          Alex Xia
 *
 * INTERFACE:           MediaPlayerModule::MediaPlayerModule(QObject *parent) : QObject(parent)
 *
 * RETURNS:             N/A, constructor
 *
 * NOTES:               This is the constructor of the class which creates an instance of a QMediaPlayer
 *                      and sets the volume to max.
 *
 ******************************************************************************/
MediaPlayerModule::MediaPlayerModule(QObject *parent) : QObject(parent)
{
    player = new QMediaPlayer;
    player->setVolume(100); //haha
}

/******************************************************************************
 * FUNCTION:          ~MediaPlayerModule
 *
 * DATE:              April 16 2018
 *
 *
 * DESIGNER:          Alex Xia
 *
 * PROGRAMMER:        Alex Xia
 *
 * INTERFACE:        MediaPlayerModule::~MediaPlayerModule()
 *
 * RETURNS:          N/A, destructor
 *
 * NOTES:           This is the destructor for the class and will remove the QMediaPlayer instance.
 *
 ******************************************************************************/
MediaPlayerModule::~MediaPlayerModule()
{
    delete player;
}

/******************************************************************************
 * FUNCTION:        Play
 *
 * DATE:            April 16 2018
 *
 *
 * DESIGNER:        Luke Lee, Vafa Dehghan Saei
 *
 * PROGRAMMER:      Luke Lee, Vafa Dehghan Saei
 *
 * INTERFACE:       void MediaPlayerModule::Play()
 *
 * RETURNS:         void
 *
 * NOTES:           This function is called on click of the play button. It will open the file specified by the user
 *                  and attempt playback if possible.
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
 * FUNCTION:        Pause
 *
 * DATE:            April 16 2018
 *
 *
 * DESIGNER:        Juliana French
 *
 * PROGRAMMER:      Juliana French
 *
 * INTERFACE:       void MediaPlayerModule::Pause()
 *
 * RETURNS:         void
 *
 * NOTES:           This function will pause the music.
 *
 ******************************************************************************/
void MediaPlayerModule::Pause()
{
    player->pause();
}

/******************************************************************************
 * FUNCTION:        Stop
 *
 * DATE:            April 16 2018
 *
 *
 * DESIGNER:        Juliana French
 *
 * PROGRAMMER:      Juliana French
 *
 * INTERFACE:       void MediaPlayerModule::Stop()
 *
 * RETURNS:         void
 *
 * NOTES:           This function will pause the music.
 *
 ******************************************************************************/
void MediaPlayerModule::Stop()
{
    player->stop();
}

/******************************************************************************
 * FUNCTION:         FastForward
 *
 * DATE:             April 16 2018
 *
 *
 * DESIGNER:        Vafa Dehghan Saei
 *
 * PROGRAMMER:      Vafa Dehghan Saei
 *
 * INTERFACE:       void MediaPlayerModule::FastForward()
 *
 * RETURNS:         void
 *
 * NOTES:           This function will set the play back speed to be 10% percent faster
 *
 ******************************************************************************/
void MediaPlayerModule::FastForward()
{
    qreal currentSpeed = player->playbackRate();
    player->setPlaybackRate(currentSpeed + 0.1);
}

/******************************************************************************
 * FUNCTION:        void MediaPlayerModule::SlowForward()
 *
 * DATE:            April 16 2018
 *
 *
 * DESIGNER:        Luke Lee
 *
 * PROGRAMMER:      Luke Lee
 *
 * INTERFACE:       void MediaPlayerModule::SlowForward()
 *
 * RETURNS:         void
 *
 * NOTES:           This function will set the play back speed to be 10% slower
 *
 ******************************************************************************/
void MediaPlayerModule::SlowForward()
{
    qreal currentSpeed = player->playbackRate();
    player->setPlaybackRate(currentSpeed - 0.1);
}

/******************************************************************************
 * FUNCTION:       ChangeSongPosition
 *
 * DATE:           April 16 2018
 *
 *
 * DESIGNER:      Alex Xia
 *
 * PROGRAMMER:    Alex Xia
 *
 * INTERFACE:     void MediaPlayerModule::ChangeSongPosition(int seconds)
 *                  int second: How many seconds to skip forward
 *
 * RETURNS:       void
 *
 * NOTES:         This function will change the song position to the given argument.
 *
 ******************************************************************************/
void MediaPlayerModule::ChangeSongPosition(int seconds)
{
    player->setPosition(seconds * 1000);
}
