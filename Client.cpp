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
            if (iCr != TIMEOUT_CR)    ClientConn();
            if (fConn)    ClientRun();
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
    errno_t err;

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
            EnableWindow(hSCtlServEdit, FALSE);
            EnableWindow(hSCtlServButton, FALSE);
            EnableWindow(hSCtlEdit, FALSE);
            EnableWindow(hSCtlButton, FALSE);
        }
        ReleaseDC(hWnd, g_hdc);
        break;
    case WM_PAINT:
        g_hdc = BeginPaint(hWnd, &paintStruct);
        InvalidateRect(hSCtlServEdit, NULL, FALSE);
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
            EnableWindow(hSCtlServEdit, FALSE);
            EnableWindow(hSCtlServButton, FALSE);
            if (!ClientConfig()) {
                EnableWindow(hSCtlServEdit, TRUE);
                EnableWindow(hSCtlServButton, TRUE);
            }
            break;
        case CLIENT_BUTTON:// 消息发送按钮
            GetWindowText(hSCtlEdit, szTmp, DEFAULT_BUFLEN);
            size_t convLen;
            err = wcstombs_s(&convLen, szSend, (size_t)DEFAULT_BUFLEN, szTmp, (size_t)DEFAULT_BUFLEN - 1);
            if (err != 0) {
                StringCchPrintf(szTmp, DEFAULT_BUFLEN, _T(FONT_FORMAT_ERROR), err);
                SCtlTextBufPush(szTmp);
            }
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

void 
PaintInit(HWND hWnd) {
    // 创建静态编辑框: 服务器地址输入
    hSCtlServEdit = CreateWindow(
        _T("Edit"),
        _T("127.0.0.1"),
        WS_CHILD | WS_BORDER | WS_VISIBLE | ES_AUTOHSCROLL | ES_AUTOVSCROLL,
        MARGIN, MARGIN, 260, 40,
        hWnd,
        (HMENU)SERVER_EDIT,
        (HINSTANCE)GetModuleHandle(NULL),
        NULL
    );
    // 创建静态按钮: 服务器地址输入确定按钮
    hSCtlServButton = CreateWindow(
        _T("Button"),
        _T(INIT_CONNECT_BUTT),
        WS_CHILD | WS_BORDER | WS_VISIBLE | BS_FLAT,
        MARGIN + 260 + MARGIN, MARGIN, 60, 40,
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
    hConn = INVALID_SOCKET;
    fConn = FALSE;
    fConf = FALSE;
    iCr = 0;

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
    SetWindowText(hSCtlText, szSCtlBuf.buf);

    return TRUE;
}

BOOL 
ClientConfig() {
    int err, iLen = szSCtlBuf.tot;
    unsigned long nonblocking = 1;
    size_t convLen;
    LPCH pMBBuf = (LPCH)malloc(MAX_IP_LEN * sizeof(CHAR));
    if (!pMBBuf)    return FALSE;
    LPTSTR szErr = (LPTSTR)malloc(iLen * sizeof(TCHAR));
    if (!szErr)    return FALSE;
    GetWindowText(hSCtlServEdit, szDefServHost, MAX_IP_LEN);

    /* 初始化 Winsock dll */
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

    /* 创建 socket */
    hConn = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (hConn == INVALID_SOCKET) {
        StringCchPrintf(szErr, iLen, _T(CREATE_SOCKET_FAIL), WSAGetLastError());
        SCtlTextBufPush(szErr);
        goto CLEAN_UP;
    }

    /* 连接服务器 */
    err = ioctlsocket(hConn, FIONBIO, &nonblocking);
    if (err == SOCKET_ERROR) {
        StringCchPrintf(szErr, iLen, _T(CONN_IOMODEL_FAIL), WSAGetLastError());
        SCtlTextBufPush(szErr);
        goto CLEAN_UP;
    }

    if (wcstombs_s(&convLen, pMBBuf, (size_t)MAX_IP_LEN, szDefServHost, (size_t)MAX_IP_LEN - 1) != 0) {// inet_addr() 参数 char*，但静态编辑框返回 wchar_t*
        free(pMBBuf);
        goto CLEAN_UP;
    }

    sockaddr_in servAddr;
    servAddr.sin_family = AF_INET;
    servAddr.sin_port = htons(DEFAULT_SERVER_PORT);
    servAddr.sin_addr.S_un.S_addr = inet_addr(pMBBuf);
    free(pMBBuf);

    err = connect(hConn, (sockaddr*)&servAddr, sizeof(servAddr));
    if (err == SOCKET_ERROR && WSAGetLastError() != WSAEWOULDBLOCK) {
        StringCchPrintf(szErr, iLen, _T(CONNECT_SERVER_FAIL), WSAGetLastError());
        SCtlTextBufPush(szErr);
        goto CLEAN_UP;
    }

    fConf = TRUE;
    free(szErr);
    return TRUE;
CLEAN_UP:
    fConf = FALSE;
    if (hConn != INVALID_SOCKET)    closesocket(hConn);
    WSACleanup();
    free(szErr);
    return FALSE;
}

void 
ClientConn() {
    if (fConf == FALSE)    return;// 如果客户端还未配置，不能连接，退出
    if (fConn == TRUE)    return;// 如果客户端已连接，也退出
    if (++iCr == TIMEOUT_CR) {// 如果客户端已经尝试了 TIMEOUT_CR 次连接都连不上
        SCtlTextBufPush(_T(CONNECT_TIMEOUT));
        EnableWindow(hSCtlServEdit, TRUE);
        EnableWindow(hSCtlServEdit, TRUE);
        return;
    }

    int iLen = szSCtlBuf.tot;
    LPTSTR szErr = (LPTSTR)malloc(iLen * sizeof(TCHAR));
    if (!szErr)    return;
    struct fd_set writefds;
    TIMEVAL tv = { 0, 0 };
    int tot;

    FD_ZERO(&writefds);
    FD_SET(hConn, &writefds);

    tot = select(0, NULL, &writefds, NULL, &tv);
    if (tot == SOCKET_ERROR) {
        StringCchPrintf(szErr, iLen, _T(SELECT_FAIL), WSAGetLastError());
        SCtlTextBufPush(szErr);
        goto CLEAN_UP;
    } else if (tot == 0)    return;

    if (FD_ISSET(hConn, &writefds))    fConn = TRUE;
    free(szErr);
    return;
CLEAN_UP:
    fConn = FALSE;
    free(szErr);
}

void 
ClientRun() {
    int err;
    size_t convLen, sendLen;
    LPTSTR szErr = (LPTSTR)malloc(DEFAULT_BUFLEN * sizeof(TCHAR));
    if (!szErr)    return;
    LPSTR szBuf = (LPSTR)malloc(DEFAULT_BUFLEN * sizeof(CHAR));
    if (!szBuf)    return;
    memset(szErr, 0, DEFAULT_BUFLEN);
    memset(szBuf, 0, DEFAULT_BUFLEN);
    struct fd_set writefds, readfds;
    TIMEVAL tv = { 0, 0 };

    FD_ZERO(&readfds);
    FD_ZERO(&writefds);
    FD_SET(hConn, &readfds);
    FD_SET(hConn, &writefds);

    err = select(0, &readfds, &writefds, NULL, &tv);
    if (err == SOCKET_ERROR) {
        StringCchPrintf(szErr, DEFAULT_BUFLEN, _T(SELECT_FAIL), WSAGetLastError());
        SCtlTextBufPush(szErr);
        goto CLEAN_UP;
    } else if (err == 0)    return;

    if (FD_ISSET(hConn, &readfds)) {
        err = recv(hConn, szRecv, DEFAULT_BUFLEN, 0);
        if (err == SOCKET_ERROR) {
            if (WSAGetLastError() != WSAEWOULDBLOCK) {
                StringCchPrintf(szErr, DEFAULT_BUFLEN, _T(MESS_RECV_FAIL), WSAGetLastError());
                SCtlTextBufPush(szErr);
            }
        } else {
            if (err == 0) {
                SCtlTextBufPush(_T(LOSS_CONNECT));
                fConn = FALSE;
                goto NEXT;
            }

            // 输出至窗口文本框（注意要宽字符转换）
            StringCchCopyA(szBuf, DEFAULT_BUFLEN, SERVER_MSG_PREFIX);
            StringCchCatA(szBuf, DEFAULT_BUFLEN, szRecv);
            mbstowcs_s(&convLen, szErr, (size_t)DEFAULT_BUFLEN, szBuf, (size_t)DEFAULT_BUFLEN - 1);
            SCtlTextBufPush(szErr);
        }
    }

NEXT:
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
        if (mbstowcs_s(&convLen, szErr, (size_t)DEFAULT_BUFLEN, szBuf, (size_t)DEFAULT_BUFLEN - 1) != 0)
            goto CLEAN_UP;
        SCtlTextBufPush(szErr);

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