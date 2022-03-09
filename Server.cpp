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
    HWND hWnd = CreateWindow(_T("CServer"), _T("Server"), WS_OVERLAPPEDWINDOW ^ WS_THICKFRAME ^ WS_MAXIMIZEBOX /* 禁止拖拽窗口尺寸、禁止最大化 */, CW_USEDEFAULT, CW_USEDEFAULT, 640, 450, NULL, NULL, hInstance, NULL);
    ShowWindow(hWnd, nCmdShow);
    UpdateWindow(hWnd);

    MSG msg = { 0 };
    while (msg.message != WM_QUIT) {
        if (PeekMessage(&msg, 0, 0, 0, PM_REMOVE)) {// NULL, 0, 0 表示接受所有信息
            // 有消息时处理消息
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        } else    ServerRun();// 无消息跑服务器
    }

    UnregisterClass(_T("CServer"), wndClass.hInstance);// 即使注册用 EX，也调用这个
    return 0;
}

LRESULT CALLBACK
WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
    int         wmId, wmEvent;      // 捕获 WM_COMMAND 消息
    PAINTSTRUCT paintStruct;
    TEXTMETRIC  tm;                 // 字体尺寸结构体
    static int	cxChar, cyChar;		// 字体尺寸

    switch (message) {
    case WM_CREATE:
        g_hdc = GetDC(hWnd);
        // 获取字体尺寸
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
        case SERVER_STARTUP_BUTT:   // Start 按钮
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
    // 创建文本框标题
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
    // 创建文本框内容
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
    // 创建编辑框
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
    // 创建确认输入按钮
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
    // 初始化文本框相关信息
    RECT rec;
    GetWindowRect(hSCtlLog, &rec);
    szSCtlBuf.col = (rec.right - rec.left) / cxChar;
    szSCtlBuf.row = (rec.bottom - rec.top) / cyChar;
    szSCtlBuf.tot = szSCtlBuf.col * szSCtlBuf.row;
    szSCtlBuf.cRow = 0;
    szSCtlBuf.buf = (LPTCH)malloc(szSCtlBuf.tot * sizeof(TCHAR));
    if (!szSCtlBuf.buf)    return FALSE;
    memset(szSCtlBuf.buf, 0, szSCtlBuf.tot);

    // 初始化默认绑定地址（cpy 会把 \0 也加上）
    StringCchCopy(szDefHost, MAX_IP_LEN, _T("127.0.0.1"));

    // 初始化 socket 相关信息
    memset(&wsaData, 0, sizeof(WSADATA));
    hListen = INVALID_SOCKET;

    // 初始化用户 socket 数组
    memset(&g_sock, 0, sizeof(SOCKINFO) * MAX_CLIENT);

    return TRUE;
}

/* Purpose: 往静态文本框(Server log)写字符串，若超出窗口大小会删除第一行字符串 */
BOOL
SCtlTextBufPush(LPCTSTR szSrc) {
    if (szSCtlBuf.cRow == szSCtlBuf.row) {
        // 如果文本框行数累计够了上限，就要删除开头一行
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
    
    // 原串追加回车符（原串是 const 所以不用释放），然后拼接至结尾
    size_t len;
    if (StringCchLength(szSrc, szSCtlBuf.col, &len) != S_OK)
        return FALSE;
    LPTCH szAppendLF = (LPTCH)malloc((len + 2) * sizeof(TCHAR));// 容纳 \n 和 \0
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
    
    // 输出至文本框
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

    /* S0: 初始化 Winsock dll */
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

    /* S1: 创建 socket */
    memset(&servHint, 0, sizeof(struct addrinfoW));
    servHint.ai_family   = AF_INET;
    servHint.ai_socktype = SOCK_STREAM;
    servHint.ai_protocol = IPPROTO_TCP;
    servHint.ai_flags    = AI_PASSIVE | AI_NUMERICHOST;

    // 第二个参数 NULL 表明端口不指定，第四个参数是取指针地址而不是结构体地址
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

    /* S2: 绑定 socket */
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

    /* S3: 监听 socket */
    if (listen(hListen, MAX_CLIENT) == SOCKET_ERROR) {
        StringCchPrintf(szErr, iLen, _T(LISTEN_SOCKET_FAIL), WSAGetLastError());
        SCtlTextBufPush(szErr);
        goto CLEAN_UP;
    }
    if (!SCtlTextBufPush(_T(LISTEN_SOCKET_SUCC)))
        goto CLEAN_UP;

    /* S4: 接受连接 */
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
    // 释放所有用户 socket 数组
    // 释放监听 socket
    // 释放 Winsock2 dll
}