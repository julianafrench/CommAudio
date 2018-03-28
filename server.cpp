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
    HANDLE ThreadHandle;
    DWORD ThreadId;
    WSAEVENT AcceptEvent;
    SOCKET AcceptSocket;

    ServerInfo *sInfo;

    //std::string svrSonglist;

    void Server::SvrConnect(ServerInfo *svrInfo)
    {
        sInfo = svrInfo;

        if ((Ret = WSAStartup(0x0202, &wsaData)) != 0)
        {
            printf("WSAStartup failed with error %d\n", Ret);
            WSACleanup();
            return;
        }

        if ((ListenSocket = WSASocket(AF_INET, SOCK_STREAM, 0, NULL, 0,
            WSA_FLAG_OVERLAPPED)) == INVALID_SOCKET)
        {
            printf("Failed to get a socket %d\n", WSAGetLastError());
            return;
        }

        InternetAddr.sin_family = AF_INET;
        InternetAddr.sin_addr.s_addr = htonl(INADDR_ANY);
        InternetAddr.sin_port = htons(DEF_PORT);

        if (bind(ListenSocket, (PSOCKADDR)&InternetAddr,
            sizeof(InternetAddr)) == SOCKET_ERROR)
        {
            printf("bind() failed with error %d\n", WSAGetLastError());
            return;
        }

        if (listen(ListenSocket, 5))
        {
            printf("listen() failed with error %d\n", WSAGetLastError());
            return;
        }

        if ((AcceptEvent = WSACreateEvent()) == WSA_INVALID_EVENT)
        {
            printf("WSACreateEvent() failed with error %d\n", WSAGetLastError());
            return;
        }

        // Create a listen thread to service completed I/O requests.
        if ((ThreadHandle = CreateThread(NULL, 0, SvrRecvThread, (LPVOID)AcceptEvent, 0, &ThreadId)) == NULL)
        {
            printf("CreateThread failed with error %d\n", GetLastError());
            return;
        }

        while (TRUE)
        {
            AcceptSocket = accept(ListenSocket, NULL, NULL);

            if (WSASetEvent(AcceptEvent) == FALSE)
            {
                printf("WSASetEvent failed with error %d\n", WSAGetLastError());
                return;
            }
        }
    }

    DWORD WINAPI Server::SvrRecvThread(LPVOID lpParameter)
    {
        DWORD Flags;
        WSAEVENT EventArray[1];
        DWORD Index;
        DWORD RecvBytes;

        // Save the accept event in the event array.
        EventArray[0] = (WSAEVENT)lpParameter;

        while (TRUE)
        {
            // Wait for accept() to signal an event and also process WorkerRoutine() returns.
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
            CreateSocketInfo(&AcceptSocket);

            sInfo->recvSocket = &AcceptSocket;

            // If file mode is selected...
            SendPlaylist(svrSocketInfo, sInfo->songlist);

            Flags = 0;
            if (WSARecv(svrSocketInfo->Socket, &(svrSocketInfo->DataBuf), 1, &RecvBytes, &Flags,
                &(svrSocketInfo->Overlapped), WorkerRoutine) == SOCKET_ERROR)
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
        //svrSonglist = "";
        char buff[BUFSIZ];
        //GetCurrentDirectory(BUFSIZ, (LPWSTR)buff);
        //std::string dir(buff);
        //dir.append("\\*.wav");
        //WIN32_FIND_DATA data;
        //HANDLE hFind;
        /*if ((hFind = FindFirstFile((LPCWSTR)dir.c_str(), &data)) != INVALID_HANDLE_VALUE)
        {
            do
            {
                svrSonglist += data.cFileName;
                svrSonglist += ";";
            } while (FindNextFile(hFind, &data) != 0);
            FindClose(hFind);
        }
        else
        {
            std::cout << "no audio files are in current directory!" << std::endl;
            return false;
        }*/

        memset(buff, 0, BUFSIZ);
        strcpy_s(buff, BUFSIZ, songlist.c_str());

        std::cout << "Sending song list: " << songlist << std::endl;

        if (send(SI->Socket, buff, BUFSIZ, 0) <= 0)
        {
            perror("send failure");
            return false;
        }

        return true;
    }


    void CALLBACK Server::WorkerRoutine(DWORD Error, DWORD BytesTransferred,
        LPWSAOVERLAPPED Overlapped, DWORD InFlags)
    {
        DWORD SendBytes, RecvBytes;
        DWORD Flags;

        // Reference the WSAOVERLAPPED structure as a SOCKET_INFORMATION structure
        SOCKET_INFORMATION *SI = (SOCKET_INFORMATION *)Overlapped;

        if (Error != 0)
        {
            printf("I/O operation failed with error %d\n", Error);
        }

        if (BytesTransferred == 0)
        {
            printf("Closing socket %d\n", SI->Socket);
        }

        if (Error != 0 || BytesTransferred == 0)
        {
            closesocket(SI->Socket);
            GlobalFree(SI);
            return;
        }

        // Check to see if the BytesRECV field equals zero. If this is so, then
        // this means a WSARecv call just completed so update the BytesRECV field
        // with the BytesTransferred value from the completed WSARecv() call.

        if (SI->BytesRECV == 0)
        {
            SI->BytesRECV = BytesTransferred;
            SI->BytesSEND = 0;
        }
        else
        {
            printf("Recvd msg: %s\n", SI->DataBuf.buf);
            SI->BytesSEND += BytesTransferred;
        }

        if (SI->BytesRECV > SI->BytesSEND)
        {

            // Post another WSASend() request.
            // Since WSASend() is not gauranteed to send all of the bytes requested,
            // continue posting WSASend() calls until all received bytes are sent.

            ZeroMemory(&(SI->Overlapped), sizeof(WSAOVERLAPPED));

            SI->DataBuf.buf = SI->Buffer + SI->BytesSEND;
            SI->DataBuf.len = SI->BytesRECV - SI->BytesSEND;

            printf("Sending msg: %s\n", SI->DataBuf.buf);

            if (WSASend(SI->Socket, &(SI->DataBuf), 1, &SendBytes, 0,
                &(SI->Overlapped), WorkerRoutine) == SOCKET_ERROR)
            {
                if (WSAGetLastError() != WSA_IO_PENDING)
                {
                    printf("WSASend() failed with error %d\n", WSAGetLastError());
                    return;
                }
            }
        }
        else
        {
            SI->BytesRECV = 0;

            // Now that there are no more bytes to send post another WSARecv() request.
            Flags = 0;
            ZeroMemory(&(SI->Overlapped), sizeof(WSAOVERLAPPED));

            SI->DataBuf.len = BUFSIZ;
            SI->DataBuf.buf = SI->Buffer;

            if (WSARecv(SI->Socket, &(SI->DataBuf), 1, &RecvBytes, &Flags,
                &(SI->Overlapped), WorkerRoutine) == SOCKET_ERROR)
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
    void Server::CreateSocketInfo(SOCKET *s)
    {
        SOCKET_INFORMATION *SI;
        char mStr[BUFSIZ];

        if ((SI = (SOCKET_INFORMATION *)malloc(sizeof(SOCKET_INFORMATION))) == NULL)
        {
            sprintf_s(mStr, "GlobalAlloc() failed with error %d\n", GetLastError());
            //MessageBox(*hwnd, mStr, "Error", MB_OK);
            return;
        }

        // Prepare SocketInfo structure for use.
        SI->Socket = *s;
        ZeroMemory(&(SI->Overlapped), sizeof(WSAOVERLAPPED));
        SI->BytesSEND = 0;
        SI->BytesRECV = 0;
        SI->DataBuf.len = BUFSIZ;
        SI->DataBuf.buf = SI->Buffer;

        svrSocketInfo = SI;
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
