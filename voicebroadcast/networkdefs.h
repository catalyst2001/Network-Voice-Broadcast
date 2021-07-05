/**
* Network Definitions
* by Kirill Deryabin
* 05.07.2021
* github.com/catalyst2001
*/
#pragma once
#include <Windows.h>
#include <stdio.h>
#pragma comment(lib, "Ws2_32.lib") //winsock static library

typedef int socket_t;

/**
* Network initializing error codes
*/
enum net_initializing_codes_e {
	NET_OK,
	NET_ERROR_INIT_API,
	NET_ERROR_OPEN_SOCKET,
	NET_ERROR_SOCKET_BIND_FAILED,
	NET_ERROR_LISTEN_FAILED
};

/**
* Creates UDP socket
*/
static int NET_Init(socket_t *ps, const char *localip, unsigned short port)
{
	WSAData wsa_data;
	if (WSAStartup(MAKEWORD(2, 2), &wsa_data) == SOCKET_ERROR) {
		printf("NET_Init: Cannot initialize Windows Sockets API\n");
		return NET_ERROR_INIT_API;
	}

	socket_t sock = socket(AF_INET, SOCK_DGRAM, NULL);
	if (sock == SOCKET_ERROR) {
		printf("NET_Init: Cannot open socket\n");
		return NET_ERROR_OPEN_SOCKET;
	}

	sockaddr_in addr;
	addr.sin_family = AF_INET;
	addr.sin_port = htons(port);
	addr.sin_addr.s_addr = (!localip || !localip[0]) ? INADDR_ANY : inet_addr(localip); //if address not set, set any address, else get address from ip string
	if (bind(sock, (sockaddr*)&addr, sizeof(addr)) == SOCKET_ERROR) {
		printf("NET_Init: Cannot bind socket to address\n");
		closesocket(sock);
		return NET_ERROR_SOCKET_BIND_FAILED;
	}
	*ps = sock;
	return NET_OK; //no errors
}

static int NET_Shutdown(socket_t sock)
{
	closesocket(sock);
	WSACleanup();
	return NET_OK;
}