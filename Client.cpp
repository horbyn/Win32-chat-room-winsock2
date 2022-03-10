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
    HWND hWnd = CreateWindow(_T("CClient"), _T("Client"), WS_OVERLAPPEDWINDOW ^ WS_THICKFRAME ^ WS_MAXIMIZEBOX /* 禁止拖拽窗口尺寸、禁止最大化 */, CW_USEDEFAULT, CW_USEDEFAULT, 400, 500, NULL, NULL, hInstance, NULL);
    ShowWindow(hWnd, nCmdShow);
    UpdateWindow(hWnd);

    MSG msg = { 0 };
    while (msg.message != WM_QUIT) {
        if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {// NULL, 0, 0 表示接受所有信息
            // 有消息时处理消息
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        } else {
            // 无消息时执行其他指令
            if (fConf && !fConn)    ClientConn();
            if (fConn && !fClose)    ClientRun();
        }
    }

    UnregisterClass(_T("CClient"), wndClass.hInstance);// 即使注册用 EX，也调用这个
    return 0;
}

LRESULT CALLBACK 
WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
    int         wmId, wmEvent;      // 捕获 WM_COMMAND 消息
    PAINTSTRUCT paintStruct;
    TEXTMETRIC  tm;                 // 字体尺寸结构体
    static int  cxChar, cyChar;

    switch (message) {
    case WM_CREATE:
        g_hdc = GetDC(hWnd);
        // 获取字体尺寸
        GetTextMetrics(g_hdc, &tm);
        cxChar = tm.tmAveCharWidth;
        cyChar = tm.tmHeight + tm.tmExternalLeading;

        PaintInit(hWnd);
        if (!ClientInit(cxChar, cyChar)) {
            SetWindowText(hSCtlText, _T(NO_AVAILABLE_MEM));
            // 如果客户端初始化失败，所有按钮失效，即只能重启应用
            EnableWindow(hSCtlServAddr, FALSE);
            EnableWindow(hSCtlServPort, FALSE);
            EnableWindow(hSCtlServButton, FALSE);
            EnableWindow(hSCtlEdit, FALSE);
            EnableWindow(hSCtlButton, FALSE);
        }
        ReleaseDC(hWnd, g_hdc);
        break;
    case WM_PAINT:
        g_hdc = BeginPaint(hWnd, &paintStruct);
        InvalidateRect(hSCtlServAddr, NULL, FALSE);
        InvalidateRect(hSCtlServPort, NULL, FALSE);
        InvalidateRect(hSCtlServButton, NULL, FALSE);
        InvalidateRect(hSCtlText, NULL, FALSE);
        InvalidateRect(hSCtlEdit, NULL, FALSE);
        InvalidateRect(hSCtlButton, NULL, FALSE);
        EndPaint(hWnd, &paintStruct);
        break;
    case WM_COMMAND:
        g_hdc = GetDC(hWnd);
        wmId = LOWORD(wParam);
        wmEvent = HIWORD(wParam);
        switch (wmId) {
        case SERVER_BUTTON:// 服务器连接按钮
            ChangeState(&fButtState);
            break;
        case CLIENT_BUTTON:// 消息发送按钮
            GetWindowTextA(hSCtlEdit, szSend, DEFAULT_BUFLEN);// 获取控件输入
            SetWindowText(hSCtlEdit, _T(""));// 每次发完消息清空控件
            break;
        default: return DefWindowProc(hWnd, message, wParam, lParam);
        }
        ReleaseDC(hWnd, g_hdc);
        break;
    case WM_KEYDOWN:
        // 按下 ESC 键
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
    SetWindowText(hSCtlText, szSCtlBuf.buf);

    return TRUE;
}

void 
PaintInit(HWND hWnd) {
    // 创建静态编辑框: 服务器地址输入
    hSCtlServAddr = CreateWindow(
        _T("Edit"),
        _T("127.0.0.1"),
        WS_CHILD | WS_BORDER | WS_VISIBLE | ES_AUTOHSCROLL | ES_AUTOVSCROLL,
        MARGIN, MARGIN, 110, 40,
        hWnd,
        (HMENU)SERVER_ADDR,
        (HINSTANCE)GetModuleHandle(NULL),
        NULL
    );
    // 创建静态编辑框: 服务器端口输入
    hSCtlServPort = CreateWindow(
        _T("Edit"),
        _T("8888"),
        WS_CHILD | WS_BORDER | WS_VISIBLE | ES_AUTOHSCROLL | ES_AUTOVSCROLL,
        MARGIN + 110 + MARGIN, MARGIN, 90, 40,
        hWnd,
        (HMENU)SERVER_PORT,
        (HINSTANCE)GetModuleHandle(NULL),
        NULL
    );
    // 创建静态按钮: 服务器地址输入确定按钮
    hSCtlServButton = CreateWindow(
        _T("Button"),
        _T(INIT_CONNECT_BUTT),
        WS_CHILD | WS_BORDER | WS_VISIBLE | BS_FLAT,
        MARGIN + 220 + MARGIN, MARGIN, 100, 40,
        hWnd,
        (HMENU)SERVER_BUTTON,
        (HINSTANCE)GetModuleHandle(NULL),
        NULL
    );
    // 创建静态文本框
    hSCtlText = CreateWindow(
        _T("Static"),
        NULL,
        WS_CHILD | WS_BORDER | WS_VISIBLE,
        MARGIN, MARGIN + 40 + MARGIN, 340, 300,
        hWnd,
        (HMENU)CLIENT_TEXT_BODY,
        (HINSTANCE)GetModuleHandle(NULL),
        NULL
    );
    // 创建静态编辑框
    hSCtlEdit = CreateWindow(
        _T("Edit"),
        _T(""),
        WS_CHILD | WS_BORDER | WS_VISIBLE | ES_AUTOHSCROLL | ES_AUTOVSCROLL,
        MARGIN, MARGIN + 360 + MARGIN, 260, 40,
        hWnd,
        (HMENU)CLIENT_EDIT,
        (HINSTANCE)GetModuleHandle(NULL),
        NULL
    );
    // 创建确认输入按钮
    hSCtlButton = CreateWindow(
        _T("Button"),
        _T(INIT_SEND_BUTT),
        WS_CHILD | WS_BORDER | WS_VISIBLE | BS_FLAT,
        MARGIN + 260 + MARGIN, MARGIN + 360 + MARGIN, 60, 40,
        hWnd,
        (HMENU)CLIENT_BUTTON,
        (HINSTANCE)GetModuleHandle(NULL),
        NULL
    );
}

BOOL 
ClientInit(int cxChar, int cyChar) {
    // 初始化文本框相关信息
    RECT rec;
    GetWindowRect(hSCtlText, &rec);
    szSCtlBuf.col = (rec.right - rec.left) / cxChar;
    szSCtlBuf.row = (rec.bottom - rec.top) / cyChar;
    szSCtlBuf.tot = szSCtlBuf.col * szSCtlBuf.row;
    szSCtlBuf.cRow = 0;
    szSCtlBuf.buf = (LPTCH)malloc(szSCtlBuf.tot * sizeof(TCHAR));
    if (!szSCtlBuf.buf)    return FALSE;
    memset(szSCtlBuf.buf, 0, szSCtlBuf.tot);

    // 初始化 socket 相关信息
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
    fConf  = FALSE;
    fConn  = FALSE;
    fClose = FALSE;
    iCr    = 0;

    /* 初始化 Winsock dll */
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

/* 设置状态为未连接。注，会关闭 socket */
void 
ResetState() {
    SetWindowText(hSCtlServButton, _T(INIT_CONNECT_BUTT));
    EnableWindow(hSCtlServAddr, TRUE);
    EnableWindow(hSCtlServPort, TRUE);
    if (hConn != INVALID_SOCKET)    closesocket(hConn);
    fButtState = 0;
    fConf  = FALSE;
    fConn  = FALSE;
    fClose = FALSE;
    iCr    = 0;
}

/* 按钮改为相反状态, 0: 服务器未连接状态; -1: 服务器已连接状态 */
void 
ChangeState(int *state) {
    if (*state == 0) {// 当前是未连接状态, 按下按钮会更改为已连接状态
        SetWindowText(hSCtlServButton, _T(CONNECT_TERMINATE));
        EnableWindow(hSCtlServAddr, FALSE);
        EnableWindow(hSCtlServPort, FALSE);
        if (!ClientConfig()) {
            SCtlTextBufPush(_T(CLIENT_CONFIG_FAIL));
            ResetState();
            return;
        }
    } else {// 当前是已连接状态, 按下按钮会更改为未连接状态
        SetWindowText(hSCtlServButton, _T(INIT_CONNECT_BUTT));
        EnableWindow(hSCtlServAddr, TRUE);
        EnableWindow(hSCtlServPort, TRUE);
        shutdown(hConn, SD_SEND);
    }

    *state = ~*state;
}

/* 创建一个新的 socket 并发起连接 */
BOOL 
ClientConfig() {
    int err, iLen = szSCtlBuf.tot;
    size_t convLen;
    int port;
    unsigned long nonblocking = 1;
    const int PORT_BIT = 11;
    TCHAR szDefServHost[MAX_IP_LEN], szDefServPort[PORT_BIT];//INT_MAX 最多 10 位
    LPTSTR szErr = (LPTSTR)malloc(iLen * sizeof(TCHAR));
    LPSTR pMBBuf = (LPSTR)malloc(MAX_IP_LEN * sizeof(CHAR));
    if (!szErr || !pMBBuf) {
        SCtlTextBufPush(_T(NO_AVAILABLE_MEM));
        return FALSE;
    }

    memset(szErr, 0, iLen);
    memset(pMBBuf, 0, MAX_IP_LEN);
    /* 创建 socket */
    hConn = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (hConn == INVALID_SOCKET) {
        StringCchPrintf(szErr, iLen, _T(CREATE_SOCKET_FAIL), WSAGetLastError());
        SCtlTextBufPush(szErr);
        goto CLEAN_UP;
    }

    /* 设为非阻塞 */
    err = ioctlsocket(hConn, FIONBIO, &nonblocking);
    if (err == SOCKET_ERROR) {
        StringCchPrintf(szErr, iLen, _T(CONN_IOMODEL_FAIL), WSAGetLastError());
        SCtlTextBufPush(szErr);
        goto CLEAN_UP;
    }

    /* 获取服务器信息 */
    GetWindowText(hSCtlServAddr, szDefServHost, MAX_IP_LEN);
    // 就算用户输入 111..111 也最多接收 9 个 1，INT_MAX 10 位也可能爆，只允许 9 个
    GetWindowText(hSCtlServPort, szDefServPort, PORT_BIT);
    port = _wtoi(szDefServPort);
    if (port < 0 || port > 65535) {
        // 用 int 来接收 short 的端口，才能知道有没有超出 short 的范围
        SCtlTextBufPush(_T(PORT_INPUT_INCORRECT));
        goto CLEAN_UP;
    }
    // inet_addr() 参数 char*，但静态编辑框返回 wchar_t*
    if (wcstombs_s(&convLen, pMBBuf, (size_t)MAX_IP_LEN, szDefServHost, (size_t)MAX_IP_LEN - 1) != 0) {
        SCtlTextBufPush(_T(FONT_FORMAT_FAIL));
        goto CLEAN_UP;
    }

    sockaddr_in servAddr;
    servAddr.sin_family = AF_INET;
    servAddr.sin_port = htons(port);
    servAddr.sin_addr.S_un.S_addr = inet_addr(pMBBuf);

    /* 连接服务器 */
    err = connect(hConn, (sockaddr*)&servAddr, sizeof(servAddr));
    if (err == SOCKET_ERROR && WSAGetLastError() != WSAEWOULDBLOCK) {
        // WSAEWOULDBLOCK: 连接需要经过三次握手，但非阻塞 socket 会立即返回
        // 这个错误码相当于服务器要等待三次握手的时间，这个时间内却接连收到连接请求
        // 所以这个错误实际上是在进行连接，只是暂时没有结果
        // 后面会一直调用 select() 捕获连接结果
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
    if (++iCr == TIMEOUT_CR) {// 如果客户端已经尝试了 TIMEOUT_CR 次连接都连不上
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

    /* 等待服务器允许连接 */
    tot = select(0, NULL, &writefds, NULL, &tv);
    if (tot == SOCKET_ERROR) {
        StringCchPrintf(szErr, iLen, _T(SELECT_FAIL), WSAGetLastError());
        SCtlTextBufPush(szErr);
        goto CLEAN_UP;
    } else if (tot == 0)    goto CLEAN_UP;// 没有连接请求

    if (FD_ISSET(hConn, &writefds))    fConn = TRUE;// 其实也不用判因为集合就一个
    
    free(szErr);
    return;
CLEAN_UP:
    fConn = FALSE;
    free(szErr);
}

void 
ClientRun() {
    int err;
    errno_t et;
    size_t convLen, sendLen;
    struct fd_set writefds, readfds;
    TIMEVAL tv = { 0, 0 };
    LPTSTR szErr = (LPTSTR)malloc(DEFAULT_BUFLEN * sizeof(TCHAR));
    LPSTR szBuf = (LPSTR)malloc(DEFAULT_BUFLEN * sizeof(CHAR));
    if (!szErr || !szBuf) {
        SCtlTextBufPush(_T(NO_AVAILABLE_MEM));
        return;
    }

    memset(szErr, 0, DEFAULT_BUFLEN);
    memset(szBuf, 0, DEFAULT_BUFLEN);
    FD_ZERO(&readfds);
    FD_ZERO(&writefds);
    FD_SET(hConn, &readfds);
    FD_SET(hConn, &writefds);

    /* 检测 IO 事件 */
    err = select(0, &readfds, &writefds, NULL, &tv);
    if (err == SOCKET_ERROR) {
        StringCchPrintf(szErr, DEFAULT_BUFLEN, _T(SELECT_FAIL), WSAGetLastError());
        SCtlTextBufPush(szErr);
        goto CLEAN_UP;
    } else if (err == 0)    return;// 无事件到达

    /* 可读事件 */
    if (FD_ISSET(hConn, &readfds)) {
        err = recv(hConn, szRecv, DEFAULT_BUFLEN, 0);
        if (err == SOCKET_ERROR) {
            // WSAEWOULDBLOCK 意味着消息正在路上，迟早会收到的，所以不检查
            if (WSAGetLastError() != WSAEWOULDBLOCK) {
                StringCchPrintf(szErr, DEFAULT_BUFLEN, _T(MESS_RECV_FAIL), WSAGetLastError());
                SCtlTextBufPush(szErr);
            }
        } else {
            if (err == 0) {
                SCtlTextBufPush(_T(CHATTING_COMPLETE));
                // 优雅关闭时，只有再接收一次 0 字节的 recv() 才算完全关闭
                if (hConn != INVALID_SOCKET)    closesocket(hConn);
                ResetState();
                return;
            }

            // 输出至窗口文本框（注意要宽字符转换）
            StringCchCopyA(szBuf, DEFAULT_BUFLEN, SERVER_MSG_PREFIX);
            StringCchCatA(szBuf, DEFAULT_BUFLEN, szRecv);
            mbstowcs_s(&convLen, szErr, (size_t)DEFAULT_BUFLEN, szBuf, (size_t)DEFAULT_BUFLEN - 1);
            SCtlTextBufPush(szErr);
        }
    }

    /* 可写事件 */
    if (FD_ISSET(hConn, &writefds)) {
        // 获取用户输入
        if (StringCchLengthA(szSend, DEFAULT_BUFLEN, &sendLen) != S_OK)
            goto CLEAN_UP;
        if (sendLen == 0)    goto CLEAN_UP;// 如果用户没输入，就不发送

        // 加上 "Client: " 的前缀再显示到文本框
        if (StringCchCopyA(szBuf, DEFAULT_BUFLEN, CLIENT_MSG_PREFIX) != S_OK)
            goto CLEAN_UP;
        if (StringCchCatA(szBuf, DEFAULT_BUFLEN, szSend) != S_OK)
            goto CLEAN_UP;
        et = mbstowcs_s(&convLen, szErr, (size_t)DEFAULT_BUFLEN, szBuf, (size_t)DEFAULT_BUFLEN - 1);
        if (et != 0) {
            StringCchPrintf(szErr, DEFAULT_BUFLEN, _T(FONT_FORMAT_ERROR), et);
            SCtlTextBufPush(szErr);
            goto CLEAN_UP;
        }
        if (!SCtlTextBufPush(szErr)) {
            // 用户输入太多（此处将不发送）
            // 但其实发送也可以，并且下一次进入本函数将会 recv() 到，但
            // 在 SCtlTextBufPush() 内有长度限制的处理，即
            // 过长而致不能一行显示全部的消息将不显示。所以实际上超过一行的消息也可以
            // 正常收发，只是此处简单处理为不显示（因为显示要考虑分行的问题）
            StringCchPrintf(szErr, DEFAULT_BUFLEN, _T(MESSAGE_TOO_LONG));
            SCtlTextBufPush(szErr);
            goto CLEAN_UP;
        }

        err = send(hConn, szSend, sendLen, 0);
        if (err == SOCKET_ERROR) {
            // WSAEWOULDBLOCK 对于发送只是缓冲区不够位置，不算出错
            if (WSAGetLastError() == WSAEWOULDBLOCK)    return;

            StringCchPrintf(szErr, DEFAULT_BUFLEN, _T(MESS_SEND_FAIL), WSAGetLastError());
            SCtlTextBufPush(szErr);
        }
    }

CLEAN_UP:
    // 收发完一次消息要清空缓冲区，不然程序逻辑发现缓冲区有内容就会调用 send()
    memset(szSend, 0, DEFAULT_BUFLEN);
    memset(szRecv, 0, DEFAULT_BUFLEN);
    free(szErr);
    free(szBuf);
}