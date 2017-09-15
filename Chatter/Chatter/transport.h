#pragma once

#ifdef WIN32
#include <winsock2.h>
#pragma comment(lib, "ws2_32.lib")
#else
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/types.h> 
#include <unistd.h>
#include <errno.h>
#include <stdio.h>
#endif

#include <vector>

#ifdef WIN32
#define GetLastError()	WSAGetLastError()
#define CloseSocket(a)	closesocket(a)
using socklen_t = int;
#else
#define GetLastError()	errno
#define CloseSocket(a)	close(a)
using SOCKET = int;
constexpr const int INVALID_SOCKET = -1;
constexpr const int SOCKET_ERROR = -1;
#endif


#ifdef WIN32

class TransportService
{
public:
	static TransportService& Instance()
	{
		static TransportService service;
		return service;
	}

	int GetStatus() const { return status; }


private:
	TransportService()
		: status(WSAStartup(MAKEWORD(2, 2), &wsaData))
	{
		if (status != 0)
		{
			printf("WSAStartup failed with error: %d\n", status);
		}
		else
		{
			printf("WinSock initialised successfully.\n");
		}
	}

	~TransportService()
	{
		WSACleanup();
		printf("WinSock API closed\n");
	}

	TransportService(const TransportService&) = delete;
	TransportService& operator= (const TransportService&) = delete;


	WSADATA wsaData;
	int status;
};

#endif




struct UdpSocket
{
	UdpSocket(const unsigned short port = 0, const char* const ip = nullptr)
		: endpoint({ 0 })
		, sockfd(INVALID_SOCKET)
	{
		endpoint.sin_family = AF_INET;
		endpoint.sin_port = htons(port);
		endpoint.sin_addr.s_addr = ip ? inet_addr(ip) : INADDR_ANY;

#ifdef WIN32
		if (TransportService::Instance().GetStatus() != 0)
			return;
#endif

		sockfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);

		if (sockfd == INVALID_SOCKET)
		{
			printf("socket failed: %d\n", GetLastError());
			return;
		}


		char flag = 1;
		setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &flag, sizeof(char));

		//non-blocking
#ifdef WIN32
		u_long mode = 1; // 1: NON-BLOCKING, 0:BLOCKING
		ioctlsocket(sockfd, FIONBIO, &mode);
#else
		struct timeval read_timeout;
		read_timeout.tv_sec = 0;
		read_timeout.tv_usec = 10;
		setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &read_timeout, sizeof read_timeout);
#endif
	}


	bool IsOpen() const { return sockfd != INVALID_SOCKET; }


	bool Bind()
	{
		if (bind(sockfd, reinterpret_cast<sockaddr*>(&endpoint), sizeof(sockaddr_in)) == SOCKET_ERROR)
		{
			printf("Bind failed with error code: %d\n", GetLastError());
			return false;
		}
		else
		{
			/* find out what port we were assigned and print it out */
			auto len = static_cast<socklen_t>(sizeof(sockaddr_in));

			if (getsockname(sockfd, reinterpret_cast<sockaddr*>(&endpoint), &len) < 0)
			{
				printf("Error getsockname: %d\n", GetLastError());
				return false;
			}

			printf("Socket<%d> bound to ip %s on port %d\n", sockfd, inet_ntoa(endpoint.sin_addr), ntohs(endpoint.sin_port));
			return true;
		}
	}


	template<class Container>
	int SendTo(const char* const destIp, const Container& data)
	{
		sockaddr_in remote = { 0 };
		remote.sin_family = AF_INET;
		remote.sin_addr.s_addr = inet_addr(destIp);
		remote.sin_port = endpoint.sin_port;
		return SendTo(remote, data);
	}


	template<class Container>
	int SendTo(const sockaddr_in& remote, const Container& data)
	{
		printf("TX: {%s;%d} (%lu bytes) => ", inet_ntoa(remote.sin_addr), ntohs(remote.sin_port), data.size());

		int rc = sendto(sockfd, reinterpret_cast<const char*>(&data[0]), data.size(), 0, reinterpret_cast<const sockaddr*>(&remote), sizeof(sockaddr_in));

		if (rc == SOCKET_ERROR)
		{
			printf("sendto() failed with error code : %d\n", GetLastError());
		}

		return rc;
	}



	template<class Container>
	std::pair<sockaddr_in, Container> RecvFrom(const size_t buffer_size)
	{
		Container data;
		data.resize(buffer_size);

		sockaddr_in remote = { 0 };
		auto len = static_cast<socklen_t>(sizeof(sockaddr_in));

		//printf("Listening on port %d...\n", ntohs(endpoint.sin_port));

		int rc = recvfrom(sockfd, reinterpret_cast<char*>(&data[0]), buffer_size, 0, reinterpret_cast<sockaddr*>(&remote), &len);

		if (rc == SOCKET_ERROR)
		{
			//printf("recvfrom() failed with error code : %d\n" , GetLastError());
			data.resize(0);
		}
		else
		{
			printf("RX: {%s;%d} (%d bytes) <= ", inet_ntoa(remote.sin_addr), ntohs(remote.sin_port), rc);
			data.resize(rc);
		}

		return std::pair<sockaddr_in, Container>(remote, data);
	}


	~UdpSocket()
	{
		CloseSocket(sockfd);
		sockfd = INVALID_SOCKET;
	}


private:
	sockaddr_in endpoint;
	SOCKET sockfd;
};

