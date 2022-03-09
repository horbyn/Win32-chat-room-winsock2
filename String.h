#pragma once

#ifndef _SERVER_STRING_H_
#define _SERVER_STRING_H_

#define INIT_PAINT_LOG		"Server current log"
#define	INIT_STARTUP_BUTT	"Startup"
#define	INIT_STARTUP_SUCC	"Winsock2 startup succeeded"
#define INIT_STARTUP_FAIL	"ERR::WSAStartup(): %d"
#define INIT_QUERY_VERSION	"ERR::WSAStartup(): query version incorrect"
#define RUN_TRANS_IP_FAIL	"ERR::GetAddrInfo(): %d"
#define RUN_TRANS_IP_SUCC	"Trans ip to NBO succeeded"
#define CREATE_SOCKET_FAIL	"ERR::socket(): %ld"
#define CREATE_SOCKET_SUCC	"Socket creation succeeded"
#define BIND_SOCKET_FAIL	"ERR::bind(): %ld"
#define BIND_SOCKET_SUCC	"Socket binding succeeded"
#define LISTEN_SOCKET_FAIL	"ERR::listen(): %ld"
#define	LISTEN_SOCKET_SUCC	"Socket is listening ..."
#define LIS_IOMODEL_FAIL	"ERR::hListen ioctlsocket(): %ld"
#define SERVER_CONFIG_SUCC	"Server configuration succeeded!"
#define USR_IOMODEL_FAIL	"ERR::sockData ioctlsocket(): %ld"
#define SELECT_FAIL			"ERR::select(): %ld"
#define	ACCEPT_FAIL			"ERR::accept(): %ld"
#define ACCEPT_SUCC			"%s connection succeeded"
#define	MESS_RECV_FAIL		"ERR::recv(): %ld"
#define	MESS_SEND_FAIL		"ERR::send(): %ld"

#endif // !_SERVER_STRING_H_