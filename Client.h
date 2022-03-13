/*
 * 针对 P2P Server，应用层需要提供自己的协议，此处自定义协议规则如下
 *
 * 自定义协议：由包头和包体两部分组成，总是固定长度（MSG_SIZE）
 *     包头逻辑上同以下结构体
 *     struct HEAD {
 *         uint8_t type; // 1B
 *         uint8_t size; // 2B
 *     };
 *     包体总是 128B
 *
 * 类型一：
 * type == 0x7f: 内含数据为用户列表
 * size:         指出包体数据实际长度
 *   ├ 1B ┼ 2B ┼     Body: 128B      ┤
 *   ┌────┬────┬─────────────────────┐
 *   │0x7f│size│     128 Bytes       │
 *   └────┴────┴─────────────────────┘
 *
 * 类型二：
 * type == 0x00: 内含数据为聊天消息
 * size:         指出包体数据实际长度
 * 包体开头 16B:  指出消息来自哪个 socket
 *   ├ 1B ┼ 2B ┼     Body: 128B      ┤
 *   ┌────┬────┬─────┬───────────────┐
 *   │0x00│size│ 16B │   MESSAGES    │
 *   └────┴────┴─────┴───────────────┘
 */

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

#define IDC_SERVER_ADDR         1
#define IDC_SERVER_PORT         2
#define IDC_SERVER_BUTTON       3
#define IDC_LOCAL_DISPLAY       4
#define IDC_CLIENT_USER_COMBO   5
#define IDC_CLIENT_TEXT_BODY    6
#define IDC_CLIENT_EDIT         7
#define IDC_CLIENT_BUTTON       8

#define MARGIN                  20
#define	DEFAULT_SERVER_HOST     "127.0.0.1"
#define	MAX_IP_LEN              16
#define TIMEOUT_CR              600000// 测得 100,000~800,000 连接等待不会过长(3.4GHz)
#define DEFAULT_BUFLEN          512
#define MAX_CLIENT              64
#define MSG_SIZE                (MAX_CLIENT * sizeof(sockaddr) / 8) + 3

#define INIT_CONNECT_BUTT       "Connect"
#define INIT_SEND_BUTT          "Send"
#define CONNECT_TERMINATE       "Terminate"
#define NO_AVAILABLE_MEM        "ERR::malloc() failed: no available mem to malloc"
#define INIT_STARTUP_FAIL       "ERR::WSAStartup(): %ld"
#define INIT_QUERY_VERSION      "ERR::WSAStartup(): query version incorrect"
#define	RUN_TRANS_IP_FAIL       "ERR::GetAddrInfo(): %ld"
#define CREATE_SOCKET_FAIL      "ERR::socket(): %ld"
#define	CONNECT_SERVER_FAIL     "ERR::connect(): %ld"
#define	LOCAL_SOCKET_FAIL       "ERR::getsockname(): %ld"
#define CONNECT_TIMEOUT         "ERR::Connection timeout"
#define CLIENT_CONFIG_FAIL      "ERR::Client configuring failed"
#define	FONT_FORMAT_FAIL        "ERR::wcstombs_s()/mbstowcs_s() failed"
#define SELECT_FAIL             "ERR::select(): %ld"
#define CONN_IOMODEL_FAIL       "ERR::ioctlsocket(): %ld"
#define PORT_INPUT_INCORRECT    "ERR::_wtoi(): Port may be incorrect"
#define	MESS_SENDTO_FAIL        "< ERR::sendto(): %ld >"
#define	MESS_RECV_FAIL          "< ERR::recv(): %ld >"
#define CHATTING_COMPLETE       "< Connection Terminated >"
#define MESSAGE_TOO_LONG        "< Message inputed too long >"
#define MEM_COPY_FAIL           "< ERR::memcpy_s(): %d >"
#define PEER_SOCKINFO           "\t%ls:%d said:"
#define PEER_MESS               "\t"
#define	FONT_FORMAT_ERROR       "< ERR:: %ld : Font cannot identify >"
#define MESS_PACK_FAIL          "< ERR:: Messages to be send packing failed >"

typedef struct tagSCtlText {
	int row, col, tot, cRow;
	LPTCH buf;
} SCTLTEXT;
typedef SCTLTEXT *LPSCTLTEXT;
typedef struct tagLocalSock {
	sockaddr_in local;
	u_short     port;
	char        *addr;
} LOCSOCK;

HDC         g_hdc = NULL;				// 全局设备环境句柄
int         cxChar, cyChar;				// 字体宽、高
HWND        hSCtlServAddr;				// 服务器地址输入框
HWND        hSCtlServPort;			    // 服务器端口输入框
HWND        hSCtlServButton;			// 服务器地址输入确认按钮
int         fButtState;					// 按钮状态: 连接/终止
HWND        hSCtlLocal;                 // 静态文本框: 本地 socket
HWND        hSCtlCombo;					// 静态列表框
HWND        hSCtlText;					// 静态文本框
HWND        hSCtlAddr;					// 静态输入框
HWND        hSCtlButton;				// 发送按钮句柄
SCTLTEXT    szSCtlBuf;					// 静态文本框缓冲区
SOCKET      hConn;						// 用于与服务器连接
BOOL        fConf;						// 已配置 socket
BOOL        fConn;						// 已连接至服务器
BOOL        fClose;						// 已主动关闭
int         iCr;						// 尝试连接次数
WSADATA     wsaData;                    // socket 信息结构体
LPSTR       szSend;                     // 发送缓冲区
LPTSTR      szTmp;						// 临时缓冲区
LOCSOCK     local;                      // 本地 socket 信息
int         idxUser;					// 当前用户索引
sockaddr_in usrList[MAX_CLIENT];        // 用户列表

LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
BOOL    SCtlTextBufPush(LPCTSTR);
void    PaintInit(HWND);
BOOL    ClientInit(int, int);
void    ResetState();
void    ChangeState(int *);
BOOL    ClientConfig();
void    ClientConn();
void    ClientRun();
BOOL    PrintMess(LPCSTR);
BOOL    UpdateUser(LPCSTR);
LPSTR   Packing(int, u_short);

#endif