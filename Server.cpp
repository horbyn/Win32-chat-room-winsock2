#include "Server.h"

int WINAPI
WinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPSTR lpCmdLine, _In_ int nCmdShow) {
    WNDCLASSEX wndClass = {
        sizeof(WNDCLASSEX),
        CS_HREDRAW | CS_VREDRAW,
        WndProc,
        0, 0,
        hInstance,
        (HICON)::LoadImage(NULL, _T("server.ico"), IMAGE_ICON, 0, 0, LR_DEFAULTSIZE | LR_LOADFROMFILE),
        LoadCursor(NULL, IDC_ARROW),
        (HBRUSH)GetStockObject(LTGRAY_BRUSH),
        NULL,
        _T("CServer"),
        NULL
    };

    if (!RegisterClassEx(&wndClass))    return -1;
    HWND hWnd = CreateWindow(_T("CServer"), _T("Server"), WS_OVERLAPPEDWINDOW ^ WS_THICKFRAME ^ WS_MAXIMIZEBOX /* ��ֹ��ק���ڳߴ硢��ֹ��� */, CW_USEDEFAULT, CW_USEDEFAULT, 640, 450, NULL, NULL, hInstance, NULL);
    ShowWindow(hWnd, nCmdShow);
    UpdateWindow(hWnd);

    MSG msg = { 0 };
    while (msg.message != WM_QUIT) {
        if (PeekMessage(&msg, 0, 0, 0, PM_REMOVE)) {// NULL, 0, 0 ��ʾ����������Ϣ
            // ����Ϣʱ������Ϣ
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        } else    ServerRun();// ����Ϣ�ܷ�����
    }

    UnregisterClass(_T("CServer"), wndClass.hInstance);// ��ʹע���� EX��Ҳ�������
    return 0;
}

LRESULT CALLBACK
WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
    int         wmId, wmEvent;      // ���� WM_COMMAND ��Ϣ
    PAINTSTRUCT paintStruct;
    TEXTMETRIC  tm;                 // ����ߴ�ṹ��
    static int	cxChar, cyChar;		// ����ߴ�

    switch (message) {
    case WM_CREATE:
        g_hdc = GetDC(hWnd);
        // ��ȡ����ߴ�
        GetTextMetrics(g_hdc, &tm);
        cxChar = tm.tmAveCharWidth;
        cyChar = tm.tmHeight + tm.tmExternalLeading;

        PaintInit(hWnd);
        ServerInit(cxChar, cyChar);
        ReleaseDC(hWnd, g_hdc);
        break;
    case WM_PAINT:
        g_hdc = BeginPaint(hWnd, &paintStruct);
        InvalidateRect(hSCtlCap, NULL, FALSE);
        InvalidateRect(hSCtlLog, NULL, FALSE);
        InvalidateRect(hSCtlEdit, NULL, FALSE);
        InvalidateRect(hSCtlStartButton, NULL, FALSE);
        EndPaint(hWnd, &paintStruct);
        break;
    case WM_COMMAND:
        g_hdc = GetDC(hWnd);
        wmId = LOWORD(wParam);
        wmEvent = HIWORD(wParam);
        switch (wmId) {
        case SERVER_STARTUP_BUTT:   // Start ��ť
            EnableWindow(hSCtlStartButton, FALSE);
            if (!ServerConfig())    EnableWindow(hSCtlStartButton, TRUE);
            break;
        default: return DefWindowProc(hWnd, message, wParam, lParam);
        }
        ReleaseDC(hWnd, g_hdc);
        break;
    case WM_DESTROY:
        PostQuitMessage(0);
        break;
    default: return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}

void
PaintInit(HWND hWnd) {
    // �����ı������
    hSCtlCap = CreateWindow(
        _T("Static"),
        _T(INIT_PAINT_LOG),
        WS_CHILD | WS_BORDER | WS_VISIBLE | SS_CENTER | SS_CENTERIMAGE,
        MARGIN, MARGIN, 580, 40,
        hWnd,
        (HMENU)SERVER_TEXT_CAPTION,
        (HINSTANCE)GetModuleHandle(NULL),
        NULL
    );
    // �����ı�������
    hSCtlLog = CreateWindow(
        _T("Static"),
        NULL,
        WS_CHILD | WS_BORDER | WS_VISIBLE | SS_LEFT,
        MARGIN, MARGIN + 40 + MARGIN, 580, 250,
        hWnd,
        (HMENU)SERVER_TEXT_BODY,
        (HINSTANCE)GetModuleHandle(NULL),
        NULL
    );
    // �����༭��
    hSCtlEdit = CreateWindow(
        _T("Edit"),
        _T("127.0.0.1"),
        WS_CHILD | WS_BORDER | WS_VISIBLE | ES_AUTOHSCROLL,
        MARGIN, MARGIN + 40 + MARGIN + 250 + MARGIN, 400, 40,
        hWnd,
        (HMENU)SERVER_EDIT,
        (HINSTANCE)GetModuleHandle(NULL),
        NULL
    );
    // ����ȷ�����밴ť
    hSCtlStartButton = CreateWindow(
        _T("Button"),
        _T(INIT_STARTUP_BUTT),
        WS_CHILD | WS_BORDER | WS_VISIBLE | BS_FLAT,
        MARGIN + 400 + MARGIN,
        MARGIN + 40 + MARGIN + 250 + MARGIN, 160, 40,
        hWnd,
        (HMENU)SERVER_STARTUP_BUTT,
        (HINSTANCE)GetModuleHandle(NULL),
        NULL
    );
}

BOOL
ServerInit(int cxChar, int cyChar) {
    // ��ʼ���ı��������Ϣ
    RECT rec;
    GetWindowRect(hSCtlLog, &rec);
    szSCtlBuf.col = (rec.right - rec.left) / cxChar;
    szSCtlBuf.row = (rec.bottom - rec.top) / cyChar;
    szSCtlBuf.tot = szSCtlBuf.col * szSCtlBuf.row;
    szSCtlBuf.cRow = 0;
    szSCtlBuf.buf = (LPTCH)malloc(szSCtlBuf.tot * sizeof(TCHAR));
    if (!szSCtlBuf.buf)    return FALSE;
    memset(szSCtlBuf.buf, 0, szSCtlBuf.tot);

    // ��ʼ��Ĭ�ϰ󶨵�ַ��cpy ��� \0 Ҳ���ϣ�
    StringCchCopy(szDefHost, MAX_IP_LEN, _T("127.0.0.1"));

    // ��ʼ�� socket �����Ϣ
    memset(&wsaData, 0, sizeof(WSADATA));
    hListen = INVALID_SOCKET;

    // ��ʼ���û� socket ����
    memset(&g_sock, 0, sizeof(SOCKINFO) * MAX_CLIENT);

    return TRUE;
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
    SetWindowText(hSCtlLog, szSCtlBuf.buf);

    return TRUE;
}

BOOL
ServerConfig() {
    int err, iLen = szSCtlBuf.tot;
    struct addrinfoW servHint, * result = NULL;
    unsigned long nonblocking = 1;
    int iTime = TIMEOUT;
    LPTSTR szErr = (LPTSTR)malloc(iLen * sizeof(TCHAR));
    if (!szErr)    return FALSE;
    GetWindowText(hSCtlEdit, szDefHost, MAX_IP_LEN);

    /* S0: ��ʼ�� Winsock dll */
    const int iQueryVersion = 2;
    err = WSAStartup(MAKEWORD(iQueryVersion, 2), &wsaData);
    if (err == 0) {
        // Startup succeed
        if (LOBYTE(wsaData.wVersion) < iQueryVersion) {
            // Version incorrect
            SCtlTextBufPush(_T(INIT_QUERY_VERSION));
            goto CLEAN_UP;
        }
    }
    else {
        // Startup failed
        StringCchPrintf(szErr, iLen, _T(INIT_STARTUP_FAIL), err);
        SCtlTextBufPush(szErr);
        free(szErr);
        return FALSE;
    }
    if (!SCtlTextBufPush(_T(INIT_STARTUP_SUCC)))
        goto CLEAN_UP;

    /* S1: ���� socket */
    memset(&servHint, 0, sizeof(struct addrinfoW));
    servHint.ai_family   = AF_INET;
    servHint.ai_socktype = SOCK_STREAM;
    servHint.ai_protocol = IPPROTO_TCP;
    servHint.ai_flags    = AI_PASSIVE | AI_NUMERICHOST;

    // �ڶ������� NULL �����˿ڲ�ָ�������ĸ�������ȡָ���ַ�����ǽṹ���ַ
    err = GetAddrInfo(szDefHost, NULL, &servHint, &result);
    if (err != 0) {
        StringCchPrintf(szErr, iLen, _T(RUN_TRANS_IP_FAIL), err);
        SCtlTextBufPush(szErr);
        goto CLEAN_UP;
    }
    if (!SCtlTextBufPush(_T(RUN_TRANS_IP_SUCC)))
        goto CLEAN_UP;

    hListen = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
    if (hListen == INVALID_SOCKET) {
        FreeAddrInfo(result);
        StringCchPrintf(szErr, iLen, _T(CREATE_SOCKET_FAIL), WSAGetLastError());
        SCtlTextBufPush(szErr);
        goto CLEAN_UP;
    }
    if (!SCtlTextBufPush(_T(CREATE_SOCKET_SUCC)))
        goto CLEAN_UP;

    /* S2: �� socket */
    err = bind(hListen, result->ai_addr, (int)result->ai_addrlen);
    if (err == SOCKET_ERROR) {
        FreeAddrInfo(result);
        StringCchPrintf(szErr, iLen, _T(BIND_SOCKET_FAIL), WSAGetLastError());
        SCtlTextBufPush(szErr);
        goto CLEAN_UP;
    }
    FreeAddrInfo(result);
    if (!SCtlTextBufPush(_T(BIND_SOCKET_SUCC)))
        goto CLEAN_UP;

    /* S3: ���� socket */
    if (listen(hListen, MAX_CLIENT) == SOCKET_ERROR) {
        StringCchPrintf(szErr, iLen, _T(LISTEN_SOCKET_FAIL), WSAGetLastError());
        SCtlTextBufPush(szErr);
        goto CLEAN_UP;
    }
    if (!SCtlTextBufPush(_T(LISTEN_SOCKET_SUCC)))
        goto CLEAN_UP;

    /* S4: �������� */
    err = ioctlsocket(hListen, FIONBIO, &nonblocking);
    if (err == SOCKET_ERROR) {
        StringCchPrintf(szErr, iLen, _T(LIS_IOMODEL_FAIL), WSAGetLastError());
        SCtlTextBufPush(szErr);
        goto CLEAN_UP;
    }

    if (!SCtlTextBufPush(_T(SERVER_CONFIG_SUCC)))
        goto CLEAN_UP;

    return TRUE;

CLEAN_UP:
    if (hListen != INVALID_SOCKET)    closesocket(hListen);
    WSACleanup();
    free(szErr);
    return FALSE;
}

void
FreeSockInfo(int i) {
    g_sockNum--;
    closesocket(g_sock[i].sock);

    for (int j = i; j < g_sockNum; ++j)
        g_sock[j] = g_sock[j + 1];
}

void
ServerRun() {
    ;
}

void
ServerFree() {
    // �ͷ������û� socket ����
    // �ͷż��� socket
    // �ͷ� Winsock2 dll
}