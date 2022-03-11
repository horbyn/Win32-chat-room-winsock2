#include "Client.h"

int WINAPI 
WinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPSTR lpCmdLine, _In_ int nCmdShow) {
    WNDCLASSEX wndClass = {
        sizeof(WNDCLASSEX),
        CS_HREDRAW | CS_VREDRAW,
        WndProc,
        0, 0,
        hInstance,
        (HICON)::LoadImage(NULL, _T("client.ico"), IMAGE_ICON, 0, 0, LR_DEFAULTSIZE | LR_LOADFROMFILE),
        LoadCursor(NULL, IDC_ARROW),
        (HBRUSH)GetStockObject(WHITE_BRUSH),
        NULL,
        _T("CClient"),
        NULL
    };

    if (!RegisterClassEx(&wndClass))    return -1;
    HWND hWnd = CreateWindow(_T("CClient"), _T("Client"), WS_OVERLAPPEDWINDOW ^ WS_THICKFRAME ^ WS_MAXIMIZEBOX /* ��ֹ��ק���ڳߴ硢��ֹ��� */, CW_USEDEFAULT, CW_USEDEFAULT, 400, 500, NULL, NULL, hInstance, NULL);
    ShowWindow(hWnd, nCmdShow);
    UpdateWindow(hWnd);

    MSG msg = { 0 };
    while (msg.message != WM_QUIT) {
        if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {// NULL, 0, 0 ��ʾ����������Ϣ
            // ����Ϣʱ������Ϣ
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        } else {
            // ����Ϣʱִ������ָ��
            if (fConf && !fConn)    ClientConn();
            if (fConn && !fClose)    ClientRun();
        }
    }

    UnregisterClass(_T("CClient"), wndClass.hInstance);// ��ʹע���� EX��Ҳ�������
    return 0;
}

LRESULT CALLBACK 
WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
    int         wmId, wmEvent;      // ���� WM_COMMAND ��Ϣ
    PAINTSTRUCT paintStruct;
    TEXTMETRIC  tm;                 // ����ߴ�ṹ��
    //int ItemIndex;
    //TCHAR  ListItem[256];

    switch (message) {
    case WM_CREATE:
        g_hdc = GetDC(hWnd);
        // ��ȡ����ߴ�
        GetTextMetrics(g_hdc, &tm);
        cxChar = tm.tmAveCharWidth;
        cyChar = tm.tmHeight + tm.tmExternalLeading;

        PaintInit(hWnd);
        if (!ClientInit(cxChar, cyChar)) {
            SetWindowText(hSCtlText, _T(NO_AVAILABLE_MEM));
            // ����ͻ��˳�ʼ��ʧ�ܣ����а�ťʧЧ����ֻ������Ӧ��
            EnableWindow(hSCtlServAddr, FALSE);
            EnableWindow(hSCtlServPort, FALSE);
            EnableWindow(hSCtlServButton, FALSE);
            EnableWindow(hSCtlCombo, FALSE);
            EnableWindow(hSCtlAddr, FALSE);
            EnableWindow(hSCtlButton, FALSE);
        }
        ReleaseDC(hWnd, g_hdc);
        break;
    case WM_PAINT:
        g_hdc = BeginPaint(hWnd, &paintStruct);
        InvalidateRect(hSCtlServAddr, NULL, FALSE);
        InvalidateRect(hSCtlServPort, NULL, FALSE);
        InvalidateRect(hSCtlServButton, NULL, FALSE);
        InvalidateRect(hSCtlCombo, NULL, FALSE);
        InvalidateRect(hSCtlText, NULL, FALSE);
        InvalidateRect(hSCtlAddr, NULL, FALSE);
        InvalidateRect(hSCtlButton, NULL, FALSE);
        EndPaint(hWnd, &paintStruct);
        break;
    case WM_COMMAND:
        g_hdc = GetDC(hWnd);
        wmId = LOWORD(wParam);
        wmEvent = HIWORD(wParam);
        switch (wmId) {
        case IDC_SERVER_BUTTON:// ���������Ӱ�ť
            ChangeState(&fButtState);
            break;
        case IDC_CLIENT_BUTTON:// ��Ϣ���Ͱ�ť
            GetWindowTextA(hSCtlAddr, szSend, DEFAULT_BUFLEN);// ��ȡ�ؼ�����
            SetWindowText(hSCtlAddr, _T(""));// ÿ�η�����Ϣ��տؼ�
            break;
        case IDC_CLIENT_USER_COMBO:// �û��б�
                switch (wmEvent) {
                case CBN_SELCHANGE:
                    if (HIWORD(wParam) == CBN_SELCHANGE)
                    {
                        //ItemIndex = SendMessage((HWND)lParam, (UINT)CB_GETCURSEL,
                        //    (WPARAM)0, (LPARAM)0);
                        //(TCHAR)SendMessage((HWND)lParam, (UINT)CB_GETLBTEXT,
                        //    (WPARAM)ItemIndex, (LPARAM)ListItem);
                    }
                    break;
                default: return DefWindowProc(hWnd, message, wParam, lParam);
                }
            break;
        default: return DefWindowProc(hWnd, message, wParam, lParam);
        }

        ReleaseDC(hWnd, g_hdc);
        break;
    case WM_KEYDOWN:
        // ���� ESC ��
        if (wParam == VK_ESCAPE)    DestroyWindow(hWnd);
        break;
    case WM_DESTROY:
        free(szSCtlBuf.buf);
        free(szTmp);
        PostQuitMessage(0);
        break;
    default: return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}

/* Purpose: ����̬�ı���(Server log)д�ַ��������������ڴ�С��ɾ����һ���ַ��� */
BOOL
SCtlTextBufPush(LPCTSTR szSrc) {
    if (szSCtlBuf.cRow == szSCtlBuf.row) {
        // ����ı��������ۼƹ������ޣ���Ҫɾ����ͷһ��
        LPTCH p = szSCtlBuf.buf;
        while (*p++ != _T('\n'));
        LPTCH szNewBuf = (LPTCH)malloc(szSCtlBuf.tot * sizeof(TCHAR));
        if (!szNewBuf)    return FALSE;
        if (StringCchCopy(szNewBuf, szSCtlBuf.tot, p) != S_OK)
            return FALSE;
        free(szSCtlBuf.buf);
        szSCtlBuf.buf = szNewBuf;
        szSCtlBuf.cRow--;
    }

    // ԭ��׷�ӻس�����ԭ���� const ���Բ����ͷţ���Ȼ��ƴ������β
    size_t len;
    if (StringCchLength(szSrc, szSCtlBuf.col, &len) != S_OK)
        return FALSE;
    LPTCH szAppendLF = (LPTCH)malloc((len + 2) * sizeof(TCHAR));// ���� \n �� \0
    if (!szAppendLF)    return FALSE;
    memset(szAppendLF, 0, len + 2);
    if (StringCchCopy(szAppendLF, len + 2, szSrc) != S_OK)
        return FALSE;
    if (StringCchCat(szAppendLF, len + 2, _T("\n")) != S_OK)
        return FALSE;
    if (StringCchCat(szSCtlBuf.buf, szSCtlBuf.tot, szAppendLF) != S_OK)
        return FALSE;
    szSCtlBuf.cRow++;
    free(szAppendLF);

    // ������ı���
    SetWindowText(hSCtlText, szSCtlBuf.buf);

    return TRUE;
}

void 
PaintInit(HWND hWnd) {
    // ������̬�༭��: ��������ַ����
    hSCtlServAddr = CreateWindow(
        _T("Edit"),
        _T("127.0.0.1"),
        WS_CHILD | WS_BORDER | WS_VISIBLE | ES_AUTOHSCROLL | ES_AUTOVSCROLL,
        MARGIN, MARGIN, 110, 40,
        hWnd,
        (HMENU)IDC_SERVER_ADDR,
        (HINSTANCE)GetModuleHandle(NULL),
        NULL
    );
    // ������̬�༭��: �������˿�����
    hSCtlServPort = CreateWindow(
        _T("Edit"),
        _T("8888"),
        WS_CHILD | WS_BORDER | WS_VISIBLE | ES_AUTOHSCROLL | ES_AUTOVSCROLL,
        MARGIN + 110 + MARGIN, MARGIN, 90, 40,
        hWnd,
        (HMENU)IDC_SERVER_PORT,
        (HINSTANCE)GetModuleHandle(NULL),
        NULL
    );
    // ������̬��ť: ��������ַ����ȷ����ť
    hSCtlServButton = CreateWindow(
        _T("Button"),
        _T(INIT_CONNECT_BUTT),
        WS_CHILD | WS_BORDER | WS_VISIBLE | BS_FLAT,
        MARGIN + 220 + MARGIN, MARGIN, 100, 40,
        hWnd,
        (HMENU)IDC_SERVER_BUTTON,
        (HINSTANCE)GetModuleHandle(NULL),
        NULL
    );
    // ������̬�б��
    hSCtlCombo = CreateWindow(
        _T("COMBOBOX"),
        NULL,
        WS_CHILD | WS_BORDER | WS_VISIBLE | LBS_STANDARD /* ������ */ | CBS_DROPDOWNLIST | CBS_HASSTRINGS,
        MARGIN, MARGIN + 40 + MARGIN, 340, cyChar * 6,
        hWnd,
        (HMENU)IDC_CLIENT_USER_COMBO,
        (HINSTANCE)GetWindowLong(hWnd, GWL_HINSTANCE),
        NULL
    );
    EnableWindow(hSCtlCombo, FALSE);
    // ������̬�ı���
    hSCtlText = CreateWindow(
        _T("Static"),
        NULL,
        WS_CHILD | WS_BORDER | WS_VISIBLE,
        MARGIN, MARGIN + 40 + MARGIN + 20 + MARGIN, 340, 260,
        hWnd,
        (HMENU)IDC_CLIENT_TEXT_BODY,
        (HINSTANCE)GetModuleHandle(NULL),
        NULL
    );
    // ������̬�༭��
    hSCtlAddr = CreateWindow(
        _T("Edit"),
        _T(""),
        WS_CHILD | WS_BORDER | WS_VISIBLE | ES_AUTOHSCROLL | ES_AUTOVSCROLL,
        MARGIN, MARGIN + 360 + MARGIN, 260, 40,
        hWnd,
        (HMENU)IDC_CLIENT_EDIT,
        (HINSTANCE)GetModuleHandle(NULL),
        NULL
    );
    // ����ȷ�����밴ť
    hSCtlButton = CreateWindow(
        _T("Button"),
        _T(INIT_SEND_BUTT),
        WS_CHILD | WS_BORDER | WS_VISIBLE | BS_FLAT,
        MARGIN + 260 + MARGIN, MARGIN + 360 + MARGIN, 60, 40,
        hWnd,
        (HMENU)IDC_CLIENT_BUTTON,
        (HINSTANCE)GetModuleHandle(NULL),
        NULL
    );
}

BOOL 
ClientInit(int cxChar, int cyChar) {
    // ��ʼ���ı��������Ϣ
    RECT rec;
    GetWindowRect(hSCtlText, &rec);
    szSCtlBuf.col = (rec.right - rec.left) / cxChar;
    szSCtlBuf.row = (rec.bottom - rec.top) / cyChar;
    szSCtlBuf.tot = szSCtlBuf.col * szSCtlBuf.row;
    szSCtlBuf.cRow = 0;
    szSCtlBuf.buf = (LPTCH)malloc(szSCtlBuf.tot * sizeof(TCHAR));
    if (!szSCtlBuf.buf)    return FALSE;
    memset(szSCtlBuf.buf, 0, szSCtlBuf.tot);

    // ��ʼ�� socket �����Ϣ
    szTmp = (LPTSTR)malloc(DEFAULT_BUFLEN * sizeof(TCHAR));
    if (!szTmp) {
        SCtlTextBufPush(_T(NO_AVAILABLE_MEM));
        return FALSE;
    }
    memset(&wsaData, 0, sizeof(WSADATA));
    memset(&szSend, 0, DEFAULT_BUFLEN);
    memset(&szRecv, 0, DEFAULT_BUFLEN);
    fButtState = 0;
    hConn  = INVALID_SOCKET;
    hPeer  = INVALID_SOCKET;
    fConf  = FALSE;
    fConn  = FALSE;
    fClose = FALSE;
    iCr    = 0;
    fSel   = FALSE;

    /* ��ʼ�� Winsock dll */
    const int iQueryVersion = 2;
    int err;
    err = WSAStartup(MAKEWORD(iQueryVersion, 2), &wsaData);
    if (err == 0) {
        // Startup succeed
        if (LOBYTE(wsaData.wVersion) < iQueryVersion) {
            // Version incorrect
            SCtlTextBufPush(_T(INIT_QUERY_VERSION));
            WSACleanup();
            return FALSE;
        }
    } else {
        // Startup failed
        StringCchPrintf(szTmp, DEFAULT_BUFLEN, _T(INIT_STARTUP_FAIL), err);
        SCtlTextBufPush(szTmp);
        return FALSE;
    }

    return TRUE;
}

/* ����״̬Ϊδ���ӡ�ע����ر� socket */
void 
ResetState() {
    SetWindowText(hSCtlServButton, _T(INIT_CONNECT_BUTT));
    EnableWindow(hSCtlServAddr, TRUE);
    EnableWindow(hSCtlServPort, TRUE);
    EnableWindow(hSCtlCombo, FALSE);
    if (hConn != INVALID_SOCKET)    closesocket(hConn);
    fButtState = 0;
    fConf  = FALSE;
    fConn  = FALSE;
    fClose = FALSE;
    fSel   = FALSE;
    iCr    = 0;
}

/* ��ť��Ϊ�෴״̬, 0: ������δ����״̬; -1: ������������״̬ */
void 
ChangeState(int *state) {
    if (*state == 0) {// ��ǰ��δ����״̬, ���°�ť�����Ϊ������״̬
        SetWindowText(hSCtlServButton, _T(CONNECT_TERMINATE));
        EnableWindow(hSCtlServAddr, FALSE);
        EnableWindow(hSCtlServPort, FALSE);
        EnableWindow(hSCtlCombo, TRUE);
        if (!ClientConfig()) {
            SCtlTextBufPush(_T(CLIENT_CONFIG_FAIL));
            ResetState();
            return;
        }
    } else {// ��ǰ��������״̬, ���°�ť�����Ϊδ����״̬
        SetWindowText(hSCtlServButton, _T(INIT_CONNECT_BUTT));
        EnableWindow(hSCtlServAddr, TRUE);
        EnableWindow(hSCtlServPort, TRUE);
        EnableWindow(hSCtlCombo, FALSE);
        shutdown(hConn, SD_SEND);
    }

    *state = ~*state;
}

/* ����һ���µ� socket ���������� */
BOOL 
ClientConfig() {
    int err, iLen = szSCtlBuf.tot;
    size_t convLen;
    int port;
    unsigned long nonblocking = 1;
    const int PORT_BIT = 11;
    sockaddr_in servAddr, peerAddr;
    TCHAR szDefServHost[MAX_IP_LEN], szDefServPort[PORT_BIT];//INT_MAX ��� 10 λ
    LPTSTR szErr = (LPTSTR)malloc(iLen * sizeof(TCHAR));
    LPSTR pMBBuf = (LPSTR)malloc(MAX_IP_LEN * sizeof(CHAR));
    if (!szErr || !pMBBuf) {
        SCtlTextBufPush(_T(NO_AVAILABLE_MEM));
        return FALSE;
    }

    memset(szErr, 0, iLen);
    memset(pMBBuf, 0, MAX_IP_LEN);
    /* ���� socket */
    // TCP
    hConn = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (hConn == INVALID_SOCKET) {
        StringCchPrintf(szErr, iLen, _T(CREATE_SOCKET_FAIL), _T("TCP"), WSAGetLastError());
        SCtlTextBufPush(szErr);
        goto CLEAN_UP;
    }
    // UDP
    hPeer = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (hConn == INVALID_SOCKET) {
        StringCchPrintf(szErr, iLen, _T(CREATE_SOCKET_FAIL), _T("UDP"), WSAGetLastError());
        SCtlTextBufPush(szErr);
        goto CLEAN_UP;
    }

    /* ��Ϊ������ */
    // TCP
    err = ioctlsocket(hConn, FIONBIO, &nonblocking);
    if (err == SOCKET_ERROR) {
        StringCchPrintf(szErr, iLen, _T(CONN_IOMODEL_FAIL), _T("TCP"), WSAGetLastError());
        SCtlTextBufPush(szErr);
        goto CLEAN_UP;
    }
    // UDP
    err = ioctlsocket(hPeer, FIONBIO, &nonblocking);
    if (err == SOCKET_ERROR) {
        StringCchPrintf(szErr, iLen, _T(CONN_IOMODEL_FAIL), _T("UDP"), WSAGetLastError());
        SCtlTextBufPush(szErr);
        goto CLEAN_UP;
    }

    /* ��ȡ��������Ϣ */
    GetWindowText(hSCtlServAddr, szDefServHost, MAX_IP_LEN);
    // �����û����� 111..111 Ҳ������ 9 �� 1��INT_MAX 10 λҲ���ܱ���ֻ���� 9 ��
    GetWindowText(hSCtlServPort, szDefServPort, PORT_BIT);
    port = _wtoi(szDefServPort);
    if (port < 0 || port > 65535) {
        // �� int ������ short �Ķ˿ڣ�����֪����û�г��� short �ķ�Χ
        SCtlTextBufPush(_T(PORT_INPUT_INCORRECT));
        goto CLEAN_UP;
    }
    // inet_addr() ���� char*������̬�༭�򷵻� wchar_t*
    if (wcstombs_s(&convLen, pMBBuf, (size_t)MAX_IP_LEN, szDefServHost, (size_t)MAX_IP_LEN - 1) != 0) {
        SCtlTextBufPush(_T(FONT_FORMAT_FAIL));
        goto CLEAN_UP;
    }

    // TCP
    servAddr.sin_family = AF_INET;
    servAddr.sin_port = htons(port);
    servAddr.sin_addr.S_un.S_addr = inet_addr(pMBBuf);
    // UDP
    peerAddr.sin_family = AF_INET;
    peerAddr.sin_port = 0;// ����˿�
    peerAddr.sin_addr.S_un.S_addr = inet_addr("127.0.0.1");// UDP �󶨵�����

    /* UDP �� */
    err = bind(hPeer, (struct sockaddr*)&peerAddr, sizeof(peerAddr));
    if (err == SOCKET_ERROR) {
        StringCchPrintf(szErr, iLen, _T(LOCAL_BOUND_FAIL), WSAGetLastError());
        SCtlTextBufPush(szErr);
        goto CLEAN_UP;
    }

    /* TCP ���ӷ����� */
    err = connect(hConn, (sockaddr*)&servAddr, sizeof(servAddr));
    if (err == SOCKET_ERROR && WSAGetLastError() != WSAEWOULDBLOCK) {
        // WSAEWOULDBLOCK: ������Ҫ�����������֣��������� socket ����������
        // ����������൱�ڷ�����Ҫ�ȴ��������ֵ�ʱ�䣬���ʱ����ȴ�����յ���������
        // �����������ʵ�������ڽ������ӣ�ֻ����ʱû�н��
        // �����һֱ���� select() �������ӽ��
        StringCchPrintf(szErr, iLen, _T(CONNECT_SERVER_FAIL), WSAGetLastError());
        SCtlTextBufPush(szErr);
        goto CLEAN_UP;
    }

    fConf  = TRUE;
    free(szErr);
    free(pMBBuf);
    return TRUE;
CLEAN_UP:
    fConf = FALSE;
    free(szErr);
    free(pMBBuf);
    return FALSE;
}

void 
ClientConn() {
    if (++iCr == TIMEOUT_CR) {// ����ͻ����Ѿ������� TIMEOUT_CR �����Ӷ�������
        SCtlTextBufPush(_T(CONNECT_TIMEOUT));
        ResetState();
        return;
    }

    int iLen = szSCtlBuf.tot, tot;
    struct fd_set writefds;
    TIMEVAL tv = { 0, 0 };
    LPTSTR szErr = (LPTSTR)malloc(iLen * sizeof(TCHAR));
    if (!szErr) {
        SCtlTextBufPush(_T(NO_AVAILABLE_MEM));
        return;
    }

    memset(szErr, 0, iLen);
    FD_ZERO(&writefds);
    FD_SET(hConn, &writefds);

    /* �ȴ��������������� */
    tot = select(0, NULL, &writefds, NULL, &tv);
    if (tot == SOCKET_ERROR) {
        StringCchPrintf(szErr, iLen, _T(SELECT_FAIL), WSAGetLastError());
        SCtlTextBufPush(szErr);
        goto CLEAN_UP;
    } else if (tot == 0)    goto CLEAN_UP;// û����������

    if (FD_ISSET(hConn, &writefds))    fConn = TRUE;// ��ʵҲ��������Ϊ���Ͼ�һ��
    
    free(szErr);
    return;
CLEAN_UP:
    fConn = FALSE;
    free(szErr);
}

void 
ClientRun() {
    int err;
    struct fd_set writefds, readfds;
    TIMEVAL tv = { 0, 0 };
    LPTSTR szErr = (LPTSTR)malloc(DEFAULT_BUFLEN * sizeof(TCHAR));
    if (!szErr) {
        SCtlTextBufPush(_T(NO_AVAILABLE_MEM));
        return;
    }

    memset(szErr, 0, DEFAULT_BUFLEN);
    FD_ZERO(&readfds);
    FD_ZERO(&writefds);
    FD_SET(hConn, &readfds);
    FD_SET(hPeer, &readfds);
    FD_SET(hPeer, &writefds);

    /* ��� IO �¼� */
    err = select(0, &readfds, &writefds, NULL, &tv);
    if (err == SOCKET_ERROR) {
        StringCchPrintf(szErr, DEFAULT_BUFLEN, _T(SELECT_FAIL), WSAGetLastError());
        SCtlTextBufPush(szErr);
        goto CLEAN_UP;
    } else if (err == 0)    return;// ���¼�����

    /* �����������µ��û��б� */
    if (FD_ISSET(hConn, &readfds)) {
        err = recv(hConn, szRecv, DEFAULT_BUFLEN, 0);
        if (err == SOCKET_ERROR) {
            // WSAEWOULDBLOCK ��ζ����Ϣ����·�ϣ�������յ��ģ����Բ����
            if (WSAGetLastError() != WSAEWOULDBLOCK) {
                StringCchPrintf(szErr, DEFAULT_BUFLEN, _T(MESS_RECV_FAIL), WSAGetLastError());
                SCtlTextBufPush(szErr);
            }
        } else {
            if (err == 0) {
                SCtlTextBufPush(_T(CHATTING_COMPLETE));
                // ���Źر�ʱ��ֻ���ٽ���һ�� 0 �ֽڵ� recv() ������ȫ�ر�
                if (hConn != INVALID_SOCKET)    closesocket(hConn);
                ResetState();
                return;
            }

            ///////////////////// �����û��б�ֻҪ��һ���û����У����ղ���ȫҲû��ϵ��
            ///////////////////// ���µ�����
            if (!UpdateUser())    goto CLEAN_UP;
        }
    }

    /////////////////// �û�ѡ����һ���б���Ŀ��ż���
    /* P2P */
    if (!fSel)    goto CLEAN_UP;// �û�û��ѡ���������
    if (FD_ISSET(hPeer, &readfds)) {
        ;
    }

    if (FD_ISSET(hPeer, &writefds)) {
        ;
    }

CLEAN_UP:
    // �շ���һ����ϢҪ��ջ���������Ȼ�����߼����ֻ����������ݾͻ���� send()
    memset(szSend, 0, DEFAULT_BUFLEN);
    memset(szRecv, 0, DEFAULT_BUFLEN);
    free(szErr);
}

BOOL 
UpdateUser() {
    ;
}