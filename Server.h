/*
 * ��� P2P Server��Ӧ�ò���Ҫ�ṩ�Լ���Э�飬�˴��Զ���Э���������
 *
 * �Զ���Э�飺�ɰ�ͷ�Ͱ�����������ɣ����ǹ̶����ȣ�MSG_SIZE��
 *     ��ͷ�߼���ͬ���½ṹ��
 *     struct HEAD {
 *         uint8_t type; // 1B
 *         uint8_t size; // 2B
 *     };
 *     �������� 128B
 *
 * ����һ��
 * type == 0x7f: �ں�����Ϊ�û��б�
 * size:         ָ����������ʵ�ʳ���
 *   �� 1B �� 2B ��     Body: 128B      ��
 *   �����������Щ��������Щ�������������������������������������������
 *   ��0x7f��size��     128 Bytes       ��
 *   �����������ة��������ة�������������������������������������������
 *
 * ���Ͷ���
 * type == 0x00: �ں�����Ϊ������Ϣ
 * size:         ָ����������ʵ�ʳ���
 * ���忪ͷ 16B:  ָ����Ϣ�����ĸ� socket
 *   �� 1B �� 2B ��     Body: 128B      ��
 *   �����������Щ��������Щ����������Щ�������������������������������
 *   ��0x00��size�� 16B ��   MESSAGES    ��
 *   �����������ة��������ة����������ة�������������������������������
 */

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

unordered_map<uint64_t, SOCKET > mp;    // ip + port ��Ӧ socket �Ĺ�ϣ��

HDC      g_hdc = NULL;					// ȫ���豸�������
HWND     hSCtlCap;                      // ��̬�ı���-����
HWND     hSCtlLog;                      // ��̬�ı���-����
HWND     hSCtlAddr;                     // ��̬�����-ip
HWND     hSCtlPort;                     // ��̬�����-�˿�
HWND     hSCtlStartButton;              // ��̬��ť-����������
SCTLTEXT szSCtlBuf;                     // ��̬�ı��򻺳���
TCHAR    szDefHost[MAX_IP_LEN];         // �󶨵�ַ
WSADATA  wsaData;                       // socket ��Ϣ�ṹ��
SOCKET   hListen;                       // ���������� socket
BOOL     fListen;                       // �Ѱ󶨲����ڼ���
SOCKINFO g_sock[MAX_CLIENT];            // �������ݴ��� socket
int      g_sockNum;                     // ������ socket ����
CHAR     usrList[MAX_CLIENT * 16];      // �û��б�

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