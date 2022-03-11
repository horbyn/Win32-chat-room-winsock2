#pragma once

// ʹ�� inet_ntoa()
#ifndef _WINSOCK_DEPRECATED_NO_WARNINGS
#define _WINSOCK_DEPRECATED_NO_WARNINGS
#endif

#ifndef _SERVER_H_
#define _SERVER_H_

#include <tchar.h>
#include <WinSock2.h>
#include <ws2tcpip.h>
#include <strsafe.h>
#include "String.h"
#pragma comment(lib, "winmm.lib")
#pragma comment(lib, "ws2_32.lib")

#define DEFAULT_BUFLEN			512
#define MAX_CLIENT				64
#define MARGIN					20
#define TIMEOUT					10000
#define MAX_IP_LEN				16

#define	IDC_SERVER_TEXT_CAPTION	1
#define	IDC_SERVER_TEXT_BODY	2
#define IDC_SERVER_SCROLL		3
#define	IDC_SERVER_STARTUP_BUTT	4
#define	IDC_SERVER_ADDR			5
#define	IDC_SERVER_PORT			6

typedef struct tagSCtlText {
	int row, col, tot, cRow;
	LPTCH buf;
} SCTLTEXT;
typedef SCTLTEXT* LPSCTLTEXT;
typedef struct tagSockInfo {
	WSABUF		wsaBuf;
	SOCKET		sock;
	sockaddr_in	uInfo;
} SOCKINFO;

HDC			g_hdc = NULL;				// ȫ���豸�������
HWND		hSCtlCap;					// ��̬�ı���-����
HWND		hSCtlLog;					// ��̬�ı���-����
HWND		hSCtlAddr;					// ��̬�����-ip
HWND		hSCtlPort;					// ��̬�����-�˿�
HWND		hSCtlStartButton;			// ��̬��ť-����������
SCTLTEXT	szSCtlBuf;					// ��̬�ı��򻺳���
TCHAR		szDefHost[MAX_IP_LEN];		// �󶨵�ַ
WSADATA		wsaData;					// socket ��Ϣ�ṹ��
SOCKET		hListen;					// ���������� socket
BOOL		fListen;					// �Ѱ󶨲����ڼ���
SOCKINFO	g_sock[MAX_CLIENT];			// �������ݴ��� socket
int			g_sockNum;					// ������ socket ����

LRESULT CALLBACK WndProc(HWND hwnd, UINT message, WPARAM wPram, LPARAM lParam);
void PaintInit(HWND);
BOOL ServerInit(int, int);
BOOL SCtlTextBufPush(LPCTSTR);
BOOL ServerConfig();
void FreeSockInfo(int);
void ServerRun();
void ServerFree();

#endif // !_SERVER_H_