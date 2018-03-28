#ifndef SOCKETSTRUCT_H
#define SOCKETSTRUCT_H

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

#endif // SOCKETSTRUCT_H
