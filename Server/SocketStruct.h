#ifndef _SocketStruct_H
#define _SocketStruct_H

#include <winsock2.h>
#include <windows.h>
#include <stdio.h>

namespace commaudio
{
	struct SOCKET_INFORMATION {
		//BOOL RecvPosted;
		OVERLAPPED Overlapped;
		char Buffer[BUFSIZ];
		WSABUF DataBuf;
		SOCKET Socket;
		DWORD BytesSEND;
		DWORD BytesRECV;
		//_SOCKET_INFORMATION *Next;
	};

	
}
#endif // !_SocketStruct_H