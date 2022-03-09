#pragma once

// 使用 inet_addr()
#ifndef _WINSOCK_DEPRECATED_NO_WARNINGS
#define _WINSOCK_DEPRECATED_NO_WARNINGS
#endif

#ifndef _CLIENT_H_
#define _CLIENT_H_

#include <tchar.h>// 使用 _T()
#include <WinSock2.h>
#include <ws2tcpip.h>
#include <strsafe.h>
#include <windows.h>
#pragma comment(lib, "winmm.lib")
#pragma comment(lib, "ws2_32.lib")

#define SERVER_EDIT				1
#define SERVER_BUTTON			2
#define CLIENT_TEXT_BODY		3
#define CLIENT_EDIT				4
#define CLIENT_BUTTON			5

#define MARGIN					20
#define	DEFAULT_SERVER_HOST		"127.0.0.1"
#define	DEFAULT_SERVER_PORT		8888// 这是服务器绑定的端口，不是 client socket 的端口
#define	MAX_IP_LEN				16
#define TIMEOUT_CR				300000// 测得 100,000~800,000 连接等待不会过长(3.4GHz)
#define DEFAULT_BUFLEN			512

#define INIT_CONNECT_BUTT		"Connect"
#define INIT_SEND_BUTT			"Send"
#define CONNECT_TERMINATE		"Terminate"
#define	FONT_FORMAT_ERROR		"< ERR:: %ld : Font cannot identify >"
#define NO_AVAILABLE_MEM		"ERR::Client init failed: no available mem to init"
#define INIT_STARTUP_FAIL		"ERR::WSAStartup(): %d"
#define INIT_QUERY_VERSION		"ERR::WSAStartup(): query version incorrect"
#define	RUN_TRANS_IP_FAIL		"ERR::GetAddrInfo(): %d"
#define CREATE_SOCKET_FAIL		"ERR::socket(): %ld"
#define	CONNECT_SERVER_FAIL		"ERR::connect(): %ld"
#define CONNECT_TIMEOUT			"ERR::connection timeout"
#define SELECT_FAIL				"ERR::select(): %ld"
#define CONN_IOMODEL_FAIL		"ERR::ioctlsocket(): %ld"
#define	MESS_SEND_FAIL			"< ERR::send(): %ld >"
#define	MESS_RECV_FAIL			"< ERR::recv(): %ld >"
#define SERVER_MSG_PREFIX		"\tServer: "
#define CLIENT_MSG_PREFIX		"Client: "
#define	LOSS_CONNECT			"< ERR::Connection lost with server >"

typedef struct tagSCtlText {
	int row, col, tot, cRow;
	LPTCH buf;
} SCTLTEXT;
typedef SCTLTEXT* LPSCTLTEXT;

HDC			g_hdc = NULL;				// 全局设备环境句柄
HWND		hSCtlServEdit;				// 服务器地址输入框
HWND		hSCtlServButton;			// 服务器地址输入确认按钮
HWND		hSCtlText;					// 静态文本框
HWND		hSCtlEdit;					// 静态输入框
HWND		hSCtlButton;				// 发送按钮句柄
SCTLTEXT	szSCtlBuf;					// 静态文本框缓冲区
SOCKET		hConn;						// 全局 socket
BOOL		fConf;						// 已配置 socket 但未连接
BOOL		fConn;						// 已连接至服务器
int			iCr;						// 尝试连接次数
WSADATA		wsaData;					// socket 信息结构体
TCHAR		szDefServHost[MAX_IP_LEN];	// 绑定地址
CHAR		szSend[DEFAULT_BUFLEN];		// 发送缓冲区
CHAR		szRecv[DEFAULT_BUFLEN];		// 接收缓冲区
LPTSTR		szTmp;						// 临时缓冲区

LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
void PaintInit(HWND);
BOOL ClientInit(int, int);
BOOL SCtlTextBufPush(LPCTSTR);
BOOL ClientConfig();
void ClientConn();
void ClientRun();

#endif