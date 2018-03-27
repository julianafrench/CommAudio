#pragma once
#pragma comment(lib, "WS2_32.lib")
#define _WINSOCK_DEPRECATED_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS

#include <winsock2.h>
#include <windows.h>
#include <stdio.h>
#include <string>
#include <iostream>
#include <vector>

#define PORT 7000
#define DATA_BUFSIZE 8192

namespace commaudio
{
	typedef struct _SOCKET_INFORMATION {
		OVERLAPPED Overlapped;
		SOCKET Socket;
		CHAR Buffer[DATA_BUFSIZE];
		WSABUF DataBuf;
		DWORD BytesSEND;
		DWORD BytesRECV;
	} SOCKET_INFORMATION, *LPSOCKET_INFORMATION;

	void SvrConnect();
	void CALLBACK WorkerRoutine(DWORD Error, DWORD BytesTransferred,
		LPWSAOVERLAPPED Overlapped, DWORD InFlags);
	DWORD WINAPI SvrRecvThread(LPVOID lpParameter);
	bool SendPlaylist(LPSOCKET_INFORMATION SI);
}
