#ifndef SERVER_H
#define SERVER_H

#pragma once
#pragma comment(lib, "WS2_32.lib")
#define _WINSOCK_DEPRECATED_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS

#include <winsock2.h>
#include <windows.h>
#include <stdio.h>
#include <string>
#include <iostream>
#include <fstream>
#include <QString>
#include <QDir>
#include <QDebug>

#define DEF_PORT 7000

namespace commaudio
{
    struct ServerInfo
    {
        //HWND *hwnd;
        int port = DEF_PORT;
        std::string songlist;
        //char sendBuffer[DATA_BUFSIZE];
        //int packetSize = DEF_P_SIZE;
        bool fileMode = true;
        bool *connected;
        SOCKET *recvSocket;
        std::ifstream fileRead;
        std::streamsize fileSize;
    };

    class Server
    {
    public:
        Server() = delete;
        static bool SvrConnect(ServerInfo *svrInfo);
        static void AcceptNewEvent();
        static DWORD WINAPI ListenThread(LPVOID lpParameter);
        static bool SendPlaylist(struct SOCKET_INFORMATION *SI, std::string songlist);
        static bool ValidateFilename(std::string filepath);
        static void CALLBACK SendFileRoutine(DWORD Error, DWORD BytesTransferred,
            LPWSAOVERLAPPED Overlapped, DWORD InFlags);
        static void Disconnect();

        static struct SOCKET_INFORMATION * CreateSocketInfo(SOCKET *s);
        static struct SOCKET_INFORMATION * GetSocketInfo(SOCKET *s);
        static void FreeSocketInfo(SOCKET *s);
    };
}

#endif // SERVER_H
