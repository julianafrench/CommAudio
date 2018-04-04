#include "Server.h"
#include "SocketStruct.h"

namespace commaudio
{
    // variables
    SOCKET_INFORMATION * svrSocketInfo;
    WSADATA wsaData;
    SOCKET ListenSocket;
    SOCKADDR_IN InternetAddr;
    INT Ret;
    HANDLE ListenThrdHwnd;
    DWORD ListenThrdId;
    WSAEVENT AcceptEvent;
    SOCKET AcceptSocket;

    ServerInfo *sInfo;

    //std::string svrSonglist;

    bool Server::SvrConnect(ServerInfo *svrInfo)
    {
        sInfo = svrInfo;

        if ((Ret = WSAStartup(0x0202, &wsaData)) != 0)
        {
            printf("WSAStartup failed with error %d\n", Ret);
            WSACleanup();
            return false;
        }

        if ((ListenSocket = WSASocket(AF_INET, SOCK_STREAM, 0, NULL, 0,
            WSA_FLAG_OVERLAPPED)) == INVALID_SOCKET)
        {
            printf("Failed to get a socket %d\n", WSAGetLastError());
            return false;
        }

        InternetAddr.sin_family = AF_INET;
        InternetAddr.sin_addr.s_addr = htonl(INADDR_ANY);
        InternetAddr.sin_port = htons(DEF_PORT);

        if (bind(ListenSocket, (PSOCKADDR)&InternetAddr,
            sizeof(InternetAddr)) == SOCKET_ERROR)
        {
            printf("bind() failed with error %d\n", WSAGetLastError());
            return false;
        }

        if (listen(ListenSocket, 5))
        {
            printf("listen() failed with error %d\n", WSAGetLastError());
            return false;
        }

        if ((AcceptEvent = WSACreateEvent()) == WSA_INVALID_EVENT)
        {
            printf("WSACreateEvent() failed with error %d\n", WSAGetLastError());
            return false;
        }

        // Create a listen thread to service completed I/O requests.
        if ((ListenThrdHwnd = CreateThread(NULL, 0, ListenThread, (LPVOID)AcceptEvent, 0, &ListenThrdId)) == NULL)
        {
            printf("CreateThread failed with error %d\n", GetLastError());
            return false;
        }

        return true;
    }

    void Server::AcceptNewEvent()
    {
        AcceptSocket = accept(ListenSocket, NULL, NULL);

        if (WSASetEvent(AcceptEvent) == FALSE)
        {
            printf("WSASetEvent failed with error %d\n", WSAGetLastError());
            return;
        }
    }

    DWORD WINAPI Server::ListenThread(LPVOID lpParameter)
    {
        DWORD Flags;
        WSAEVENT EventArray[1];
        DWORD Index;
        DWORD RecvBytes;

        // Save the accept event in the event array.
        EventArray[0] = (WSAEVENT)lpParameter;

        while (TRUE)
        {
            // Wait for accept() to signal an event and also process SendFileRoutine() returns.
            while (TRUE)
            {
                Index = WSAWaitForMultipleEvents(1, EventArray, FALSE, WSA_INFINITE, TRUE);

                if (Index == WSA_WAIT_FAILED)
                {
                    printf("WSAWaitForMultipleEvents failed with error %d\n", WSAGetLastError());
                    return FALSE;
                }

                if (Index != WAIT_IO_COMPLETION)
                {
                    // An accept() call event is ready - break the wait loop
                    break;
                }
            }

            WSAResetEvent(EventArray[Index - WSA_WAIT_EVENT_0]);

            // Create a socket information structure to associate with the accepted socket.
            // note: this is not necessary in this case
            svrSocketInfo = CreateSocketInfo(&AcceptSocket);

            sInfo->recvSocket = &AcceptSocket;

            // If file mode is selected...
            SendPlaylist(svrSocketInfo, sInfo->songlist);

            Flags = 0;
            if (WSARecv(svrSocketInfo->Socket, &(svrSocketInfo->DataBuf), 1, &RecvBytes, &Flags,
                &(svrSocketInfo->Overlapped), SendFileRoutine) == SOCKET_ERROR)
            {
                if (WSAGetLastError() != WSA_IO_PENDING)
                {
                    printf("WSARecv() failed with error %d\n", WSAGetLastError());
                    return FALSE;
                }
            }

            printf("Socket %d connected\n", AcceptSocket);
        }

        return TRUE;
    }

    bool Server::SendPlaylist(SOCKET_INFORMATION *SI, std::string songlist)
    {
        char buff[BUFSIZ];
        memset(buff, 0, BUFSIZ);
        strcpy_s(buff, BUFSIZ, songlist.c_str());

        //std::cout << "Sending song list: " << songlist << std::endl;

        if (send(SI->Socket, buff, BUFSIZ, 0) <= 0)
        {
            perror("send failure");
            return false;
        }

        return true;
    }

    bool Server::ValidateFilename(std::string filepath)
    {
        sInfo->fileRead.open(filepath, std::ios::binary | std::ios::ate);
        sInfo->fileSize = sInfo->fileRead.tellg();
        sInfo->fileRead.seekg(0, std::ios::beg);
        //char buff[1024];
        if (sInfo->fileSize < 0) // no file found (tellg() returns -1)
        {
            return false;
        }
        return true;
    }


    void CALLBACK Server::SendFileRoutine(DWORD Error, DWORD BytesTransferred,
        LPWSAOVERLAPPED Overlapped, DWORD InFlags)
    {
        DWORD SendBytes, RecvBytes;
        DWORD Flags;

        // Reference the WSAOVERLAPPED structure as a SOCKET_INFORMATION structure
        SOCKET_INFORMATION *SI = (SOCKET_INFORMATION *)Overlapped;

        if (Error != 0)
        {
            //printf("I/O operation failed with error %d\n", Error);
            FreeSocketInfo(&SI->Socket);
            return;
        }

        // BytesTransferred means actual bytes received
        if (BytesTransferred == 0)
        {
            //printf("Closing socket %d\n", SI->Socket);
            FreeSocketInfo(&SI->Socket);
            return;
        }

        // Check to see if the BytesRECV field equals zero. If this is so, then
        // this means a WSARecv call just completed so update the BytesRECV field
        // with the BytesTransferred value from the completed WSARecv() call.

        if (SI->BytesToSEND == 0)
        {
            std::string selectedFile = "";
            selectedFile += svrSocketInfo->DataBuf.buf;
            if (!ValidateFilename(selectedFile))
            {
                // error msg on file not found
                return;
            }
            SI->BytesToSEND = sInfo->fileSize;
            SI->SendBuff = new char(sInfo->fileSize);
            SI->BytesSENT = 0;
        }
        else
        {
            //SI->BytesSEND += BytesTransferred;
        }

        if (SI->BytesToSEND > 0)
        {

            // Post another WSASend() request.
            // Since WSASend() is not gauranteed to send all of the bytes requested,
            // continue posting WSASend() calls until all bytes in file are sent.

            ZeroMemory(&(SI->Overlapped), sizeof(WSAOVERLAPPED));

            //SI->DataBuf.buf = SI->Buffer + SI->BytesSEND;
            //SI->DataBuf.len = SI->BytesRECV - SI->BytesSEND;

            if (!sInfo->fileRead.read(SI->Buffer, DATA_BUFSIZE))
            {
                // error msg for readfile failure
                return;
            }
            //SI->DataBuf.buf = SI->Buffer;
            //SI->DataBuf.len = DATA_BUFSIZE;

            SI->DataBuf.buf = SI->SendBuff;
            SI->DataBuf.len = SI->BytesToSEND - SI->BytesSENT;

            printf("Sending msg: %s\n", SI->DataBuf.buf);

            if (WSASend(SI->Socket, &(SI->DataBuf), 1, &SendBytes, 0,
                &(SI->Overlapped), SendFileRoutine) == SOCKET_ERROR)
            {
                if (WSAGetLastError() != WSA_IO_PENDING)
                {
                    qDebug() << "WSASend() failed with error " << WSAGetLastError();
                    return;
                }
            }
            SI->BytesSENT += SendBytes;
            SI->BytesToSEND -= SendBytes;
        }
        else
        {
            SI->BytesToSEND = 0;
            sInfo->fileRead.close();

            // Now that there are no more bytes to send post another WSARecv() request.
            Flags = 0;
            ZeroMemory(&(SI->Overlapped), sizeof(WSAOVERLAPPED));

            SI->DataBuf.len = DATA_BUFSIZE;
            SI->DataBuf.buf = SI->Buffer;

            if (WSARecv(SI->Socket, &(SI->DataBuf), 1, &RecvBytes, &Flags,
                &(SI->Overlapped), SendFileRoutine) == SOCKET_ERROR)
            {
                if (WSAGetLastError() != WSA_IO_PENDING)
                {
                    printf("WSARecv() failed with error %d\n", WSAGetLastError());
                    return;
                }
            }
        }
    }

    /*----------------------------------------------------------------------
    -- FUNCTION:	CreateSocketInfo
    --
    -- DATE:		February 14, 2018
    --
    -- DESIGNER:	Luke Lee
    --
    -- PROGRAMMER:	Luke Lee
    --
    -- INTERFACE:	void CreateSocketInfo(SOCEKT *s)
    --
    -- ARGUMENT:	*s				- pointer to a socket
    --
    -- RETURNS:		void
    --
    -- NOTES:
    -- This function creates a Socket Info struct corresponding to *s.
    ----------------------------------------------------------------------*/
    SOCKET_INFORMATION * Server::CreateSocketInfo(SOCKET *s)
    {
        SOCKET_INFORMATION *SI;
        char mStr[BUFSIZ];

        if ((SI = (SOCKET_INFORMATION *)malloc(sizeof(SOCKET_INFORMATION))) == NULL)
        {
            sprintf_s(mStr, "GlobalAlloc() failed with error %d\n", GetLastError());
            //MessageBox(*hwnd, mStr, "Error", MB_OK);
            return nullptr;
        }

        // Prepare SocketInfo structure for use.
        SI->Socket = *s;
        ZeroMemory(&(SI->Overlapped), sizeof(WSAOVERLAPPED));
        SI->BytesSENT = 0;
        SI->BytesToSEND = 0;
        SI->BytesRECV = 0;
        SI->DataBuf.len = DATA_BUFSIZE;
        SI->DataBuf.buf = SI->Buffer;

        return SI;
    }

    /*----------------------------------------------------------------------
    -- FUNCTION:	GetSocketInfo
    --
    -- DATE:		February 14, 2018
    --
    -- DESIGNER:	Luke Lee
    --
    -- PROGRAMMER:	Luke Lee
    --
    -- INTERFACE:	void GetSocketInfo(SOCEKT *s)
    --
    -- ARGUMENT:	*s				- pointer to a socket
    --
    -- RETURNS:		LPSOCKET_INFORMATION
    --
    -- NOTES:
    -- This function gets and returns the Socket Info struct corresponding
    -- to *s.
    ----------------------------------------------------------------------*/
    SOCKET_INFORMATION * Server::GetSocketInfo(SOCKET *s)
    {
        if (svrSocketInfo->Socket == *s)
        {
            return svrSocketInfo;
        }
        return nullptr;
    }

    /*----------------------------------------------------------------------
    -- FUNCTION:	FreeSocketInfo
    --
    -- DATE:		February 14, 2018
    --
    -- DESIGNER:	Luke Lee
    --
    -- PROGRAMMER:	Luke Lee
    --
    -- INTERFACE:	void FreeSocketInfo(SOCEKT *s)
    --
    -- ARGUMENT:	*s				- pointer to a socket
    --
    -- RETURNS:		LPSOCKET_INFORMATION
    --
    -- NOTES:
    -- This function takes in a pointer to a socket and free up the memory
    -- for the Socket Info struct associated with that socket.
    ----------------------------------------------------------------------*/
    void Server::FreeSocketInfo(SOCKET *s)
    {
        closesocket(*s);
        if (svrSocketInfo != nullptr)
        {
            free(svrSocketInfo);
        }
        WSACleanup();
    }
}
