#ifndef CLIENT_H
#define CLIENT_H

#pragma comment(lib, "WS2_32.lib")

#define _WINSOCK_DEPRECATED_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS

#define DEF_PORT 7000

#include <winsock2.h>	// winsock2.h needs to be include before windows.h, otherwise will have redefinition problem
#include <windows.h>
#include <string>
#include <iostream>
#include <fstream>

namespace commaudio
{
    struct ClientInfo
    {
        HWND *hwnd;
        std::string server_input;
        std::string songlist;
        int port = DEF_PORT;
        bool isIPAddress = true;
        char *sendBuffer;
        //int packetSize = DEF_P_SIZE;
        bool fileMode = true;
        bool *connected;
        SOCKET *sendSocket;
    };

    class Client
    {
    public:
        Client() = delete;
        static bool ClntConnect(ClientInfo *clntInfo);
        //static void ClntProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
        static bool ReceivePlaylist();

        static void CreateSocketInfo(SOCKET *s);
        static struct SOCKET_INFORMATION * GetSocketInfo(SOCKET *s);
        static void FreeSocketInfo(SOCKET *s);
    };
}

#endif // CLIENT_H
