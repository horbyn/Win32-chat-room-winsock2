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

// ʹ�� inet_addr()
#ifndef _WINSOCK_DEPRECATED_NO_WARNINGS
#define _WINSOCK_DEPRECATED_NO_WARNINGS
#endif

#ifndef _CLIENT_H_
#define _CLIENT_H_

#include <tchar.h>// ʹ�� _T()
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
#define TIMEOUT_CR              600000// ��� 100,000~800,000 ���ӵȴ��������(3.4GHz)
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

HDC         g_hdc = NULL;				// ȫ���豸�������
int         cxChar, cyChar;				// �������
HWND        hSCtlServAddr;				// ��������ַ�����
HWND        hSCtlServPort;			    // �������˿������
HWND        hSCtlServButton;			// ��������ַ����ȷ�ϰ�ť
int         fButtState;					// ��ť״̬: ����/��ֹ
HWND        hSCtlLocal;                 // ��̬�ı���: ���� socket
HWND        hSCtlCombo;					// ��̬�б��
HWND        hSCtlText;					// ��̬�ı���
HWND        hSCtlAddr;					// ��̬�����
HWND        hSCtlButton;				// ���Ͱ�ť���
SCTLTEXT    szSCtlBuf;					// ��̬�ı��򻺳���
SOCKET      hConn;						// ���������������
BOOL        fConf;						// ������ socket
BOOL        fConn;						// ��������������
BOOL        fClose;						// �������ر�
int         iCr;						// �������Ӵ���
WSADATA     wsaData;                    // socket ��Ϣ�ṹ��
LPSTR       szSend;                     // ���ͻ�����
LPTSTR      szTmp;						// ��ʱ������
LOCSOCK     local;                      // ���� socket ��Ϣ
int         idxUser;					// ��ǰ�û�����
sockaddr_in usrList[MAX_CLIENT];        // �û��б�

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