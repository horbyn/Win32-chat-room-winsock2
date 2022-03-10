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

#define SERVER_ADDR				1
#define SERVER_PORT				2
#define SERVER_BUTTON			3
#define CLIENT_TEXT_BODY		4
#define CLIENT_EDIT				5
#define CLIENT_BUTTON			6

#define MARGIN					20
#define	DEFAULT_SERVER_HOST		"127.0.0.1"
#define	MAX_IP_LEN				16
#define TIMEOUT_CR				600000// 测得 100,000~800,000 连接等待不会过长(3.4GHz)
#define DEFAULT_BUFLEN			512

#define INIT_CONNECT_BUTT		"Connect"
#define INIT_SEND_BUTT			"Send"
#define CONNECT_TERMINATE		"Terminate"
#define	FONT_FORMAT_ERROR		"< ERR:: %ld : Font cannot identify >"
#define NO_AVAILABLE_MEM		"ERR::malloc() failed: no available mem to malloc"
#define INIT_STARTUP_FAIL		"ERR::WSAStartup(): %d"
#define INIT_QUERY_VERSION		"ERR::WSAStartup(): query version incorrect"
#define	RUN_TRANS_IP_FAIL		"ERR::GetAddrInfo(): %d"
#define CREATE_SOCKET_FAIL		"ERR::socket(): %ld"
#define	CONNECT_SERVER_FAIL		"ERR::connect(): %ld"
#define CONNECT_TIMEOUT			"ERR::Connection timeout"
#define CLIENT_CONFIG_FAIL		"ERR::Client configuring failed"
#define	FONT_FORMAT_FAIL		"ERR::wcstombs_s()/mbstowcs_s() failed"
#define SELECT_FAIL				"ERR::select(): %ld"
#define CONN_IOMODEL_FAIL		"ERR::ioctlsocket(): %ld"
#define PORT_INPUT_INCORRECT	"ERR::_wtoi(): Port may be incorrect"
#define	MESS_SEND_FAIL			"< ERR::send(): %ld >"
#define	MESS_RECV_FAIL			"< ERR::recv(): %ld >"
#define SERVER_MSG_PREFIX		"\tServer: "
#define CLIENT_MSG_PREFIX		"Client: "
#define CHATTING_COMPLETE		"< Connection Terminated >"
#define MESSAGE_TOO_LONG		"< Message inputed too long >"

typedef struct tagSCtlText {
	int row, col, tot, cRow;
	LPTCH buf;
} SCTLTEXT;
typedef SCTLTEXT* LPSCTLTEXT;

HDC			g_hdc = NULL;				// 全局设备环境句柄
HWND		hSCtlServAddr;				// 服务器地址输入框
HWND		hSCtlServPort;				// 服务器端口输入框
HWND		hSCtlServButton;			// 服务器地址输入确认按钮
int			fButtState;					// 按钮状态: 连接/终止
HWND		hSCtlText;					// 静态文本框
HWND		hSCtlEdit;					// 静态输入框
HWND		hSCtlButton;				// 发送按钮句柄
SCTLTEXT	szSCtlBuf;					// 静态文本框缓冲区
SOCKET		hConn;						// 全局 socket
BOOL		fConf;						// 已配置 socket
BOOL		fConn;						// 已连接至服务器
BOOL		fClose;						// 已主动关闭
int			iCr;						// 尝试连接次数
WSADATA		wsaData;					// socket 信息结构体
CHAR		szSend[DEFAULT_BUFLEN];		// 发送缓冲区
CHAR		szRecv[DEFAULT_BUFLEN];		// 接收缓冲区
LPTSTR		szTmp;						// 临时缓冲区

LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
BOOL SCtlTextBufPush(LPCTSTR);
void PaintInit(HWND);
BOOL ClientInit(int, int);
void ResetState();
void ChangeState(int *);
BOOL ClientConfig();
void ClientConn();
void ClientRun();

#endif