#include "Client.h"
#include "SocketStruct.h"

namespace commaudio
{
    SOCKET_INFORMATION * clntSocketInfo;
    ClientInfo *cInfo;
    HWND *hwnd;
    struct sockaddr_in server;
    bool file_recv_complete = false;

    SOCKET sdSocket;

    FILE *fileWrite;

    bool Client::ClntConnect(ClientInfo *clntInfo)
    {
        WSADATA stWSAData;
        WORD wVersionRequested = MAKEWORD(2, 2);
        DWORD Ret;
        struct hostent	*hp;
        char mStr[BUFSIZ];

        cInfo = clntInfo;
        //hwnd = cInfo->hwnd;

        const char* host = cInfo->server_input.c_str();

        // Initialize the DLL with version Winsock 2.2
        WSAStartup(wVersionRequested, &stWSAData);

        // Create the socket
        if ((sdSocket = socket(AF_INET, SOCK_STREAM, 0)) == INVALID_SOCKET)
        {
            sprintf_s(mStr, "Cannot create socket.");
            //MessageBox(*hwnd, mStr, "Error", MB_OK);
            FreeSocketInfo(&sdSocket);
            return false;
        }

        cInfo->sendSocket = &sdSocket;

        // Initialize and set up the address structure
        memset((char *)&server, 0, sizeof(struct sockaddr_in));
        server.sin_family = AF_INET;
        server.sin_port = htons
        (cInfo->port);
        if (cInfo->isIPAddress)
        {
            struct in_addr* addr_p;
            addr_p = (struct in_addr*)malloc(sizeof(struct in_addr));
            addr_p->s_addr = inet_addr(host);
            if ((hp = gethostbyaddr((char *)addr_p, PF_INET, sizeof(&addr_p))) == NULL)
            {
                sprintf_s(mStr, "Unknown server address.");
                //MessageBox(*hwnd, mStr, "Error", MB_OK);
                FreeSocketInfo(&sdSocket);
                return false;
            }
        }
        else
        {
            if ((hp = gethostbyname(host)) == NULL)
            {
                sprintf_s(mStr, "Unknown server name.");
                //MessageBox(*hwnd, mStr, "Error", MB_OK);
                FreeSocketInfo(&sdSocket);
                return false;
            }
        }

        // Copy the server address
        memcpy((char *)&server.sin_addr, hp->h_addr, hp->h_length);

        //WSAAsyncSelect(sdSocket, *clntInfo->hwnd, WM_SOCKET, FD_CONNECT | FD_WRITE | FD_CLOSE);

        // Connecting to the server
        if ((Ret = WSAConnect(sdSocket, (struct sockaddr *)&server, sizeof(server), 0, 0, 0, NULL)) != 0)
        {
            if ((Ret = WSAGetLastError()) != WSAEWOULDBLOCK)
            {
                sprintf_s(mStr, "Can't connect to server: %d", Ret);
                //MessageBox(*hwnd, mStr, "Error", MB_OK);
                FreeSocketInfo(&sdSocket);
                return false;
            }
            // else, connection is still in progress
        }
        else
        {
            //fprintf(stdout, "Connected to server %s on port %d", server, clntInfo->port);
            fprintf(stdout, "Connected:    Server Name: %s\n", hp->h_name);
            fprintf(stdout, "\t\tIP Address: %s\n", inet_ntoa(server.sin_addr));
            //return true;
        }
        clntSocketInfo = CreateSocketInfo(&sdSocket);

        //if (cInfo->fileMode)
        //{
        //    if(ReceivePlaylist())
        //    {
                // update songlist on UI
        //        std::cout << "Song List: " << cInfo->songlist << std::endl;
        //    }
        //}

        return true;
    }

    void Client::ReceivePlaylist()
    {
        DWORD flags;
        DWORD recvBytes;

        cInfo->songlist = "";
        flags = 0;
        if (WSARecv(clntSocketInfo->Socket, &(clntSocketInfo->DataBuf), 1, &recvBytes, &flags, NULL, NULL) == SOCKET_ERROR)
        {
            if (WSAGetLastError() != WSA_IO_PENDING)
            {
                printf("WSARecv() failed with error %d\n", WSAGetLastError());
                return;
            }
        }

        cInfo->songlist += clntSocketInfo->DataBuf.buf;
    }

    bool Client::SendFilename(std::string filename)
    {
        cInfo->selFilename = filename;
        int sentBytes;
        char buff[BUFSIZ];
        memset(buff, 0, BUFSIZ);
        strcpy_s(buff, BUFSIZ, filename.c_str());

        if (sentBytes = send(clntSocketInfo->Socket, buff, BUFSIZ, 0) <= 0)
        {
            // error message
            qDebug() << "SendFilename failed";
            return false;
        }

        return true;
    }

    void Client::ReceiveFileSetup()
    {
        WSAEVENT RecvEvent;
        HANDLE ClntRecvThrdHwnd;
        DWORD ClntThrdId;

        if ((RecvEvent = WSACreateEvent()) == WSA_INVALID_EVENT)
        {
            printf("WSACreateEvent() failed with error %d\n", WSAGetLastError());
            return;
        }

        // Create a recv thread to start receiving file contents from server.
        if ((ClntRecvThrdHwnd = CreateThread(NULL, 0, ClntRecvThread, (LPVOID)RecvEvent, 0, &ClntThrdId)) == NULL)
        {
            //printf("CreateThread failed with error %d\n", GetLastError());
            return;
        }
    }

    DWORD WINAPI Client::ClntRecvThread(LPVOID lpParameter)
    {
        DWORD Flags;
        WSAEVENT EventArray[1];
        DWORD Index;
        DWORD RecvBytes;

        Flags = 0;
        if (WSARecv(clntSocketInfo->Socket, &(clntSocketInfo->DataBuf), 1, &RecvBytes, &Flags,
            &(clntSocketInfo->Overlapped), RecvFileRoutine) == SOCKET_ERROR)
        {
            if (WSAGetLastError() != WSA_IO_PENDING)
            {
                printf("WSARecv() failed with error %d\n", WSAGetLastError());
                return FALSE;
            }
        }

        // Save the recv event in the event array.
        EventArray[0] = (WSAEVENT)lpParameter;

        while (TRUE)
        {
            // Wait for accept() to signal an event and also process SendFileRoutine() returns.
            while (TRUE)
            {
                Index = WSAWaitForMultipleEvents(1, EventArray, FALSE, WSA_INFINITE, TRUE);

                if (Index == WAIT_IO_COMPLETION)
                {
                    // An overlapped request completion routine just completed. Continue servicing more completion routines.
                    continue;
                }
                else
                {
                    printf("WSAWaitForMultipleEvents failed with error %d\n", WSAGetLastError());
                    return FALSE;
                }
            }
        }

        return TRUE;
    }

    void CALLBACK Client::RecvFileRoutine(DWORD Error, DWORD BytesTransferred,
                LPWSAOVERLAPPED Overlapped, DWORD InFlags)
    {
        DWORD RecvBytes;
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
            // server closes socket
            //printf("Closing socket %d\n", SI->Socket);
            FreeSocketInfo(&SI->Socket);
            return;
        }

        if (SI->BytesRECV == 0)
        {
            // first package received
            QByteArray temp(SI->Buffer, DATA_BUFSIZE);
            WriteToFile(cInfo->selFilename, temp);
        }
        else
        {
            QByteArray temp(SI->Buffer, DATA_BUFSIZE);
            AppendToFile(cInfo->selFilename, temp);
        }
        SI->BytesRECV += BytesTransferred;

        Flags = 0;
        ZeroMemory(&(SI->Overlapped), sizeof(WSAOVERLAPPED));

        SI->DataBuf.len = DATA_BUFSIZE;
        SI->DataBuf.buf = SI->Buffer;

        if (WSARecv(SI->Socket, &(SI->DataBuf), 1, &RecvBytes, &Flags,
            &(SI->Overlapped), RecvFileRoutine) == SOCKET_ERROR)
        {
            if (WSAGetLastError() != WSA_IO_PENDING)
            {
                printf("WSARecv() failed with error %d\n", WSAGetLastError());
                return;
            }
        }
        return;
    }

    /*----------------------------------------------------------------------
    -- FUNCTION:	WriteToFile
    --
    -- DATE:		February 14, 2018
    --
    -- DESIGNER:	Luke Lee
    --
    -- PROGRAMMER:	Luke Lee
    --
    -- INTERFACE:	bool WriteToFile(std::string filePath, char *buffer)
    --
    -- ARGUMENT:	filePath		- a string path to file
    --				buffer			- array of char
    --
    -- RETURNS:		bool			- returns true if succeffully write to
    --								  file
    --
    -- NOTES:
    -- This function writes the received buffer into a file specified by
    -- user.
    ----------------------------------------------------------------------*/
    bool Client::WriteToFile(std::string filename, QByteArray buffer)
    {
        //std::fstream fileWrite(filename, std::fstream::in | std::fstream::out | std::fstream::app);
        fileWrite = fopen(filename.c_str(),"wb");
        //if (fileWrite.is_open())
        if (fileWrite)
        {
            //fileWrite << buffer;
            fwrite(buffer, 1, buffer.size(), fileWrite);
            fclose(fileWrite);
            return true;
        }
        return false;
    }

    bool Client::AppendToFile(std::string filename, QByteArray buffer)
    {
        /*
        std::ofstream outfile;

        outfile.open(filename, std::ios_base::app);
        if (outfile.is_open())
        {
            outfile << buffer;
            outfile.close();
            return true;
        }
        return false;*/

        fileWrite = fopen(filename.c_str(), "ab");
        if (fileWrite)
        {
            fwrite(buffer, 1, buffer.size(), fileWrite);
            fclose(fileWrite);
            return true;
        }
        return false;
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
    -- INTERFACE:	void CreateSocketInfo(SOCKET *s)
    --
    -- ARGUMENT:	*s				- pointer to a socket
    --
    -- RETURNS:		void
    --
    -- NOTES:
    -- This function creates a Socket Info struct corresponding to *s.
    ----------------------------------------------------------------------*/
    SOCKET_INFORMATION * Client::CreateSocketInfo(SOCKET *s)
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
    SOCKET_INFORMATION * Client::GetSocketInfo(SOCKET *s)
    {
        if (clntSocketInfo->Socket == *s)
        {
            return clntSocketInfo;
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
    void Client::FreeSocketInfo(SOCKET *s)
    {
        closesocket(*s);
        if (clntSocketInfo != nullptr)
        {
            free(clntSocketInfo);
        }
        WSACleanup();
    }
}
