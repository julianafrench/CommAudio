#include "iosocketpair.h"

/*------------------------------------------------------------------------------------------------------------------
-- SOURCE FILE: iosocketpair.cpp - Struct-like class
--
-- PROGRAM: CommAudio Networked Media Player
--
-- DATE: April 11, 2018
--
-- DESIGNER: Alex Xia, Juliana French, Luke Lee, Vafa Saei
--
-- PROGRAMMER: Alex Xia
--
-- FUNCTIONS:
--      IOSocketPair(QObject *parent, QAudioFormat* format)
--      ~IOSocketPair()
--
-- NOTES:
-- This class started as a pair of sockets for IO purposes. It is essentially a QObject struct that contains 
-- all the member variables needed for a single streaming connection. It includes :
--      - a Sending (client) socket
--      - a Receiving(server listening) socket
--      - a Pointer to a QAudioOutput device interface, either speaker or null 
--      - a Pointer to a QAudioinput device interface, either mic or audio file
-- This is used by StreamingModule to track multiple simultaneous client connections.
----------------------------------------------------------------------------------------------------------------------*/

/*------------------------------------------------------------------------------------------------------------------
-- FUNCTION IOSocketPair constructor
--
-- DATE: April 11, 2018
--
-- DESIGNER: Alex Xia, Juliana French, Luke Lee, Vafa Saei
--
-- PROGRAMMER: Alex Xia
--
-- INTERFACE: IOSocketPair::IOSocketPair(QObject *parent, QAudioFormat* format)
--
-- RETURNS: N/A
--
-- NOTES:
-- Call this function to create a new IOSocketPair object
-- As it includes pointers to audio devices, it needs a preset QAudioFormat(sample rate, sample size, etc)
-- to tune the audio devices with. Also initializes speake volume to 100% since initial testing found it
-- very quiet.
----------------------------------------------------------------------------------------------------------------------*/
IOSocketPair::IOSocketPair(QObject *parent, QAudioFormat* format) : QObject(parent)
{
    input = new QAudioInput(*format, this);
    output = new QAudioOutput(*format, this);
    output->setVolume(1.0);

    recvSocket = nullptr;

    sendSocket = new QTcpSocket;
    sendStream = new QDataStream(sendSocket);
    //cant connect slots here, funky include error
}

/*------------------------------------------------------------------------------------------------------------------
-- FUNCTION IOSocketPair destructor
--
-- DATE: April 11, 2018
--
-- DESIGNER: Alex Xia, Juliana French, Luke Lee, Vafa Saei
--
-- PROGRAMMER: Alex Xia
--
-- INTERFACE: IOSocketPair::~IOSocketPair()
--
-- RETURNS: N/A
--
-- NOTES:
-- Deallocs an IOSocketPair object.
-- Stops speaker & microphone first before disconnecting their respective sockets.
-- Let Qt handle deletion of all member variables, as many of their may have data left in their buffers to free up.
----------------------------------------------------------------------------------------------------------------------*/
IOSocketPair::~IOSocketPair()
{
    input->stop();
    output->stop();
    if (sendSocket != nullptr)
    {
        sendSocket->readAll();
        sendSocket->disconnect();
        sendSocket->disconnectFromHost();
        sendSocket->deleteLater();
    }
    if (recvSocket != nullptr)
    {
        recvSocket->readAll();
        recvSocket->deleteLater();
    }
    if (sendStream != nullptr)
        delete sendStream;
    output->deleteLater();
    input->deleteLater();
    recvSocket->deleteLater();
}
