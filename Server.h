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

// 使用 inet_ntoa()
#ifndef _WINSOCK_DEPRECATED_NO_WARNINGS
#define _WINSOCK_DEPRECATED_NO_WARNINGS
#endif

#ifndef _SERVER_H_
#define _SERVER_H_

#include <tchar.h>
#include <WinSock2.h>
#include <ws2tcpip.h>
#include <strsafe.h>
#include <unordered_map>
#include "String.h"
#pragma comment(lib, "winmm.lib")
#pragma comment(lib, "ws2_32.lib")
using namespace std;

#define DEFAULT_BUFLEN          512
#define MAX_CLIENT              64
#define MARGIN                  20
#define TIMEOUT                 10000
#define MAX_IP_LEN              16
#define MSG_SIZE                (MAX_CLIENT * sizeof(sockaddr) / 8) + 3

#define	IDC_SERVER_TEXT_CAPTION 1
#define	IDC_SERVER_TEXT_BODY    2
#define IDC_SERVER_SCROLL       3
#define	IDC_SERVER_STARTUP_BUTT 4
#define	IDC_SERVER_ADDR         5
#define	IDC_SERVER_PORT         6

typedef struct tagSCtlText {
	int row, col, tot, cRow;
	LPTCH buf;
} SCTLTEXT;
typedef SCTLTEXT* LPSCTLTEXT;
typedef struct tagSockInfo {
	LPSTR       buf;
	SOCKET      sock;
	sockaddr_in	uInfo;
} SOCKINFO;

unordered_map<uint64_t, SOCKET > mp;    // ip + port 对应 socket 的哈希表

HDC      g_hdc = NULL;					// 全局设备环境句柄
HWND     hSCtlCap;                      // 静态文本框-标题
HWND     hSCtlLog;                      // 静态文本框-内容
HWND     hSCtlAddr;                     // 静态输入框-ip
HWND     hSCtlPort;                     // 静态输入框-端口
HWND     hSCtlStartButton;              // 静态按钮-启动服务器
SCTLTEXT szSCtlBuf;                     // 静态文本框缓冲区
TCHAR    szDefHost[MAX_IP_LEN];         // 绑定地址
WSADATA  wsaData;                       // socket 信息结构体
SOCKET   hListen;                       // 服务器监听 socket
BOOL     fListen;                       // 已绑定并正在监听
SOCKINFO g_sock[MAX_CLIENT];            // 所有数据传输 socket
int      g_sockNum;                     // 已连接 socket 数量
CHAR     usrList[MAX_CLIENT * 16];      // 用户列表

LRESULT CALLBACK WndProc(HWND hwnd, UINT message, WPARAM wPram, LPARAM lParam);
void    PaintInit(HWND);
BOOL    ServerInit(int, int);
BOOL    SCtlTextBufPush(LPCTSTR);
BOOL    ServerConfig();
void    FreeSockInfo(int);
void    ServerRun();
void    ServerFree();
void    TransferMess(int);
BOOL    SendUserList();

#endif // !_SERVER_H_