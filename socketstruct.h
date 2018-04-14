#ifndef SOCKETSTRUCT_H
#define SOCKETSTRUCT_H

#define DATA_BUFSIZE        25600
#define INVALID_FILE_MSG    "Invalid filename"

#include <winsock2.h>
#include <windows.h>
#include <stdio.h>

namespace commaudio
{
    struct SOCKET_INFORMATION {
        //BOOL RecvPosted;
        OVERLAPPED Overlapped;
        char *TempBuff;
        char Buffer[DATA_BUFSIZE];
        WSABUF DataBuf;
        SOCKET Socket;
        DWORD BytesRECV = 0;
        DWORD BytesSENT = 0;
        DWORD BytesToSEND = 0;
        //_SOCKET_INFORMATION *Next;
    };
}

#endif // SOCKETSTRUCT_H
