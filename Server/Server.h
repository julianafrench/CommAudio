#pragma once
#pragma comment(lib, "WS2_32.lib")
#define _WINSOCK_DEPRECATED_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS

#include <winsock2.h>
#include <windows.h>
#include <stdio.h>
#include <string>
#include <iostream>

#define DEF_PORT 7000

namespace commaudio
{
	class Server
	{
	public:
		Server() = delete;
		static void SvrConnect();
		static DWORD WINAPI SvrRecvThread(LPVOID lpParameter);
		static bool SendPlaylist(struct SOCKET_INFORMATION *SI);
		static void CALLBACK WorkerRoutine(DWORD Error, DWORD BytesTransferred,
			LPWSAOVERLAPPED Overlapped, DWORD InFlags);

		static void CreateSocketInfo(SOCKET *s);
		static struct SOCKET_INFORMATION * GetSocketInfo(SOCKET *s);
		static void FreeSocketInfo(SOCKET *s);
	};	
}
