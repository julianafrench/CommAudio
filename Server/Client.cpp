#include "Client.h"
#include "SocketStruct.h"

namespace commaudio
{
	SOCKET_INFORMATION * clntSocketInfo;
	ClientInfo *cInfo;
	HWND *hwnd;
	struct sockaddr_in server;

	SOCKET sdSocket;

	std::string clntSonglist;

	bool Client::ClntConnect(ClientInfo *clntInfo)
	{
		WSADATA stWSAData;
		WORD wVersionRequested = MAKEWORD(2, 2);
		DWORD Ret;
		struct hostent	*hp;
		char mStr[BUFSIZ];

		cInfo = clntInfo;
		hwnd = cInfo->hwnd;

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
		CreateSocketInfo(&sdSocket);

		if (cInfo->fileMode)
		{
			clntSonglist = ReceivePlaylist();
			// update songlist on UI
			std::cout << "Song List: " << clntSonglist << std::endl;
		}

		return true;
	}

	std::string Client::ReceivePlaylist()
	{
		DWORD flags;
		DWORD recvBytes;

		std::string playlist = "";
		flags = 0;
		if (WSARecv(clntSocketInfo->Socket, &(clntSocketInfo->DataBuf), 1, &recvBytes, &flags, NULL, NULL) == SOCKET_ERROR)
		{
			if (WSAGetLastError() != WSA_IO_PENDING)
			{
				printf("WSARecv() failed with error %d\n", WSAGetLastError());
				return false;
			}
		}

		playlist += clntSocketInfo->DataBuf.buf;
		return playlist;
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
	void Client::CreateSocketInfo(SOCKET *s)
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

		clntSocketInfo = SI;
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