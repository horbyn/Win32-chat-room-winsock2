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
        InvalidateRect(hSCtlLocal, NULL, FALSE);
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
        case IDC_SERVER_BUTTON:// 服务器连接按钮
            ChangeState(&fButtState);
            break;
        case IDC_CLIENT_BUTTON:// 消息发送按钮
            if (idxUser != -1) {
                GetWindowTextA(hSCtlAddr, szSend, DEFAULT_BUFLEN);// 获取控件输入
                SetWindowText(hSCtlAddr, _T(""));// 每次发完消息清空控件
            }
            break;
        case IDC_CLIENT_USER_COMBO:// 用户列表
                switch (wmEvent) {
                case CBN_SELCHANGE:
                    if (HIWORD(wParam) == CBN_SELCHANGE) {
                        idxUser = SendMessage((HWND)lParam, (UINT)CB_GETCURSEL, (WPARAM)0, (LPARAM)0);
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
        // 按下 ESC 键
        if (wParam == VK_ESCAPE)    DestroyWindow(hWnd);
        break;
    case WM_DESTROY:
        free(szSend);
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
        MARGIN, MARGIN, 110, 30,
        hWnd,
        (HMENU)IDC_SERVER_ADDR,
        (HINSTANCE)GetModuleHandle(NULL),
        NULL
    );
    // 创建静态编辑框: 服务器端口输入
    hSCtlServPort = CreateWindow(
        _T("Edit"),
        _T("8888"),
        WS_CHILD | WS_BORDER | WS_VISIBLE | ES_AUTOHSCROLL | ES_AUTOVSCROLL,
        MARGIN + 110 + MARGIN, MARGIN, 90, 30,
        hWnd,
        (HMENU)IDC_SERVER_PORT,
        (HINSTANCE)GetModuleHandle(NULL),
        NULL
    );
    // 创建静态按钮: 服务器地址输入确定按钮
    hSCtlServButton = CreateWindow(
        _T("Button"),
        _T(INIT_CONNECT_BUTT),
        WS_CHILD | WS_BORDER | WS_VISIBLE | BS_FLAT,
        MARGIN + 220 + MARGIN, MARGIN, 100, 30,
        hWnd,
        (HMENU)IDC_SERVER_BUTTON,
        (HINSTANCE)GetModuleHandle(NULL),
        NULL
    );
    // 创建静态文本框
    hSCtlLocal = CreateWindow(
        _T("Static"),
        NULL,
        WS_CHILD | WS_BORDER | WS_VISIBLE,
        MARGIN, MARGIN + 30 + MARGIN, 340, 30,
        hWnd,
        (HMENU)IDC_LOCAL_DISPLAY,
        (HINSTANCE)GetModuleHandle(NULL),
        NULL
    );
    // 创建静态列表框
    hSCtlCombo = CreateWindow(
        _T("COMBOBOX"),
        NULL,
        WS_CHILD | WS_BORDER | WS_VISIBLE | LBS_STANDARD /* 滚动条 */ | CBS_DROPDOWNLIST | CBS_HASSTRINGS,
        MARGIN, MARGIN + 30 + MARGIN + 30 + MARGIN, 340, cyChar * 6,
        hWnd,
        (HMENU)IDC_CLIENT_USER_COMBO,
        (HINSTANCE)GetWindowLong(hWnd, GWL_HINSTANCE),
        NULL
    );
    EnableWindow(hSCtlCombo, FALSE);
    // 创建静态文本框
    hSCtlText = CreateWindow(
        _T("Static"),
        NULL,
        WS_CHILD | WS_BORDER | WS_VISIBLE,
        MARGIN, MARGIN + 30 + MARGIN + 30 + MARGIN + 20 + MARGIN, 340, 230,
        hWnd,
        (HMENU)IDC_CLIENT_TEXT_BODY,
        (HINSTANCE)GetModuleHandle(NULL),
        NULL
    );
    // 创建静态编辑框
    hSCtlAddr = CreateWindow(
        _T("Edit"),
        _T(""),
        WS_CHILD | WS_BORDER | WS_VISIBLE | ES_AUTOHSCROLL | ES_AUTOVSCROLL,
        MARGIN, MARGIN + 370 + MARGIN, 260, 30,
        hWnd,
        (HMENU)IDC_CLIENT_EDIT,
        (HINSTANCE)GetModuleHandle(NULL),
        NULL
    );
    // 创建确认输入按钮
    hSCtlButton = CreateWindow(
        _T("Button"),
        _T(INIT_SEND_BUTT),
        WS_CHILD | WS_BORDER | WS_VISIBLE | BS_FLAT,
        MARGIN + 260 + MARGIN, MARGIN + 370 + MARGIN, 60, 30,
        hWnd,
        (HMENU)IDC_CLIENT_BUTTON,
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
    if (!szSCtlBuf.buf) {
        SCtlTextBufPush(_T(NO_AVAILABLE_MEM));
        return FALSE;
    }
    memset(szSCtlBuf.buf, 0, szSCtlBuf.tot);

    // 初始化 socket 相关信息
    szTmp = (LPTSTR)malloc(DEFAULT_BUFLEN * sizeof(TCHAR));
    szSend = (LPSTR)malloc(DEFAULT_BUFLEN * sizeof(CHAR));
    if (!szTmp || !szSend) {
        SCtlTextBufPush(_T(NO_AVAILABLE_MEM));
        return FALSE;
    }
    memset(&wsaData, 0, sizeof(WSADATA));
    memset(szSend, 0, DEFAULT_BUFLEN);
    memset(usrList, 0, MAX_CLIENT * sizeof(struct sockaddr_in));
    fButtState  = 0;
    hConn   = INVALID_SOCKET;
    fConf   = FALSE;
    fConn   = FALSE;
    fClose  = FALSE;
    iCr     = 0;
    idxUser = -1;

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
    SendMessageA(hSCtlCombo, (UINT)CB_RESETCONTENT, (WPARAM)0, (LPARAM)0);// 重设
    EnableWindow(hSCtlServAddr, TRUE);
    EnableWindow(hSCtlServPort, TRUE);
    EnableWindow(hSCtlCombo, FALSE);
    if (hConn != INVALID_SOCKET)    closesocket(hConn);
    fButtState = 0;
    fConf   = FALSE;
    fConn   = FALSE;
    fClose  = FALSE;
    iCr     = 0;
    idxUser = -1;
}

/* 按钮改为相反状态, 0: 服务器未连接状态; -1: 服务器已连接状态 */
void 
ChangeState(int *state) {
    if (*state == 0) {// 当前是未连接状态, 按下按钮会更改为已连接状态
        memset(szSCtlBuf.buf, 0, szSCtlBuf.tot);
        szSCtlBuf.cRow = 0;
        SetWindowText(hSCtlText, _T(""));
        SetWindowText(hSCtlServButton, _T(CONNECT_TERMINATE));
        EnableWindow(hSCtlServAddr, FALSE);
        EnableWindow(hSCtlServPort, FALSE);
        EnableWindow(hSCtlCombo, TRUE);
        if (!ClientConfig()) {
            SCtlTextBufPush(_T(CLIENT_CONFIG_FAIL));
            ResetState();
            return;
        }
    } else {// 当前是已连接状态, 按下按钮会更改为未连接状态
        SetWindowText(hSCtlServButton, _T(INIT_CONNECT_BUTT));
        EnableWindow(hSCtlServAddr, TRUE);
        EnableWindow(hSCtlServPort, TRUE);
        EnableWindow(hSCtlCombo, FALSE);
        shutdown(hConn, SD_SEND);
    }

    *state = ~*state;
}

/* 创建一个新的 socket 并发起连接 */
BOOL 
ClientConfig() {
    int err, iLen = szSCtlBuf.tot, port;
    size_t convLen;
    unsigned long nonblocking = 1;
    const int PORT_BIT = 11;
    sockaddr_in servAddr;
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
    // inet_addr() 参数 char*，但静态编辑框返回 wchar_t*，所以宽字符需要转换
    if (wcstombs_s(&convLen, pMBBuf, (size_t)MAX_IP_LEN, szDefServHost, (size_t)MAX_IP_LEN - 1) != 0) {
        SCtlTextBufPush(_T(FONT_FORMAT_FAIL));
        goto CLEAN_UP;
    }

    servAddr.sin_family = AF_INET;
    servAddr.sin_port = htons(port);
    servAddr.sin_addr.S_un.S_addr = inet_addr(pMBBuf);

    /* 连接服务器 */
    err = connect(hConn, (sockaddr*)&servAddr, sizeof(servAddr));
    if (err == SOCKET_ERROR && WSAGetLastError() != WSAEWOULDBLOCK) {
        // WSAEWOULDBLOCK: 连接需要经过三次握手，但非阻塞 socket 会立即返回
        // 这个错误码相当于服务器要等待三次握手的时间，而这个时间内却接连收到连接请求
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

    int iLen = szSCtlBuf.tot, tot, metaLen, err;
    size_t convLen;
    sockaddr_in sockInfo;
    struct fd_set writefds;
    TIMEVAL tv = { 0, 0 };
    LPTSTR pWCBuf = (LPTSTR)malloc((MAX_IP_LEN + 1) * sizeof(TCHAR));
    LPTSTR szErr = (LPTSTR)malloc(iLen * sizeof(TCHAR));
    if (!szErr || !pWCBuf) {
        SCtlTextBufPush(_T(NO_AVAILABLE_MEM));
        return;
    }

    memset(szErr, 0, iLen);
    memset(pWCBuf, 0, MAX_IP_LEN + 1);
    FD_ZERO(&writefds);
    FD_SET(hConn, &writefds);

    /* 等待服务器允许连接 */
    tot = select(0, NULL, &writefds, NULL, &tv);
    if (tot == SOCKET_ERROR) {
        StringCchPrintf(szErr, iLen, _T(SELECT_FAIL), WSAGetLastError());
        SCtlTextBufPush(szErr);
        goto CLEAN_UP;
    } else if (tot == 0)    goto CLEAN_UP;// 没有连接请求

    if (FD_ISSET(hConn, &writefds)) {// 其实也不用判因为集合就一个
        fConn = TRUE;

        // 连上服务器后便保存本地信息
        metaLen = sizeof(struct sockaddr_in);
        err = getsockname(hConn, (sockaddr*)&sockInfo, &metaLen);
        if (err != 0) {
            StringCchPrintf(szErr, iLen, _T(LOCAL_SOCKET_FAIL), WSAGetLastError());
            SCtlTextBufPush(szErr);
            return;
        }
        local.port = ntohs(sockInfo.sin_port);
        local.addr = inet_ntoa(sockInfo.sin_addr);

        if (mbstowcs_s(&convLen, pWCBuf, MAX_IP_LEN + 1, local.addr, MAX_IP_LEN) != 0)    goto CLEAN_UP;
        StringCchPrintf(szErr, iLen, _T("Local: %s:%d"), pWCBuf, local.port);
        SetWindowText(hSCtlLocal, szErr);
    }
    
    free(szErr);
    free(pWCBuf);
    return;
CLEAN_UP:
    fConn = FALSE;
    free(szErr);
    free(pWCBuf);
}

void 
ClientRun() {
    int err, tot;
    size_t convLen, sendLen;
    errno_t et;
    struct fd_set writefds, readfds;
    TIMEVAL tv = { 0, 0 };
    LPSTR  szPack = NULL;
    LPTSTR szErr  = (LPTSTR)malloc(DEFAULT_BUFLEN * sizeof(TCHAR));
    LPSTR  szBuf  = (LPSTR)malloc(DEFAULT_BUFLEN * sizeof(CHAR));
    LPSTR  szRecv = (LPSTR)malloc(MSG_SIZE * sizeof(CHAR));
    if (!szErr || !szBuf || !szRecv) {
        SCtlTextBufPush(_T(NO_AVAILABLE_MEM));
        return;
    }

    memset(szErr, 0, DEFAULT_BUFLEN);
    memset(szBuf, 0, DEFAULT_BUFLEN);
    memset(szRecv, 0, MSG_SIZE);
    FD_ZERO(&readfds);
    FD_ZERO(&writefds);
    FD_SET(hConn, &readfds);
    FD_SET(hConn, &writefds);

    /* 检测 IO 事件 */
    tot = select(0, &readfds, &writefds, NULL, &tv);
    if (tot == SOCKET_ERROR) {
        StringCchPrintf(szErr, DEFAULT_BUFLEN, _T(SELECT_FAIL), WSAGetLastError());
        SCtlTextBufPush(szErr);
        goto CLEAN_UP;
    } else if (tot == 0)    return;// 无事件到达

    /* 检测服务器更新的用户列表、或者接收服务器的转发消息 */
    if (FD_ISSET(hConn, &readfds)) {
        err = recv(hConn, szRecv, MSG_SIZE, 0);
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
                ResetState();
                return;
            }

            // 判头字节: 0x00 == 转发聊天; 0xff == 更新用户列表
            if (szRecv[0] == 0x7f) {
                // 上线新用户、原用户下线都会触发
                if (!UpdateUser(szRecv))    goto CLEAN_UP;
            } else {
                if (!PrintMess(szRecv))    goto CLEAN_UP;
            }
        }
    }

    // 用户没有选择聊天对象
    if (idxUser == -1)    goto CLEAN_UP;

    /* p2p write */
    if (FD_ISSET(hConn, &writefds)) {
        // 获取用户输入
        if (StringCchLengthA(szSend, DEFAULT_BUFLEN, &sendLen) != S_OK)
            goto CLEAN_UP;
        if (sendLen == 0)    goto CLEAN_UP;// 如果用户没输入，就不发送
        else if ((int)sendLen > szSCtlBuf.col) {
            // 用户输入太多（此处将不发送）
            // 但其实发送也可以，并且下一次进入本函数将能 recv() 得到，但
            // 在 SCtlTextBufPush() 内有长度限制的处理，即
            // 过长而致不能一行显示全部的消息将不显示。所以实际上超过一行的消息也可以
            // 正常收发，只是此处简单处理为不显示（因为显示要考虑分行的问题）
            StringCchPrintf(szErr, DEFAULT_BUFLEN, _T(MESSAGE_TOO_LONG));
            SCtlTextBufPush(szErr);
            goto CLEAN_UP;
        }

        // 用户输入显示到文本框
        if (StringCchCopyA(szBuf, DEFAULT_BUFLEN, szSend) != S_OK)
            goto CLEAN_UP;
        et = mbstowcs_s(&convLen, szErr, (size_t)DEFAULT_BUFLEN, szBuf, (size_t)DEFAULT_BUFLEN - 1);
        if (et != 0) {
            StringCchPrintf(szErr, DEFAULT_BUFLEN, _T(FONT_FORMAT_ERROR), et);
            SCtlTextBufPush(szErr);
            goto CLEAN_UP;
        }
        SCtlTextBufPush(szErr);

        // 封装协议
        szPack = Packing(idxUser, (u_short)sendLen);
        if (!szPack) {
            StringCchPrintf(szErr, DEFAULT_BUFLEN, _T(MESS_PACK_FAIL));
            SCtlTextBufPush(szErr);
            goto CLEAN_UP;
        }

        err = send(hConn, szPack, MSG_SIZE, 0);
        if (err == SOCKET_ERROR) {
            // WSAEWOULDBLOCK 意味着消息正在路上，迟早会收到的，所以不检查
            if (WSAGetLastError() == WSAEWOULDBLOCK)    return;

            StringCchPrintf(szErr, DEFAULT_BUFLEN, _T(MESS_SENDTO_FAIL), WSAGetLastError());
            SCtlTextBufPush(szErr);
        }
        memset(szSend, 0, DEFAULT_BUFLEN);// 发送完消息清空缓冲区
    }
    idxUser = -1;

CLEAN_UP:
    free(szErr);
    free(szBuf);
    if (szPack)    free(szPack);
    free(szRecv);
}

BOOL 
PrintMess(LPCSTR recv) {
    const int metaLen = sizeof(struct sockaddr);
    unsigned short size;
    TCHAR pWCBuf[MSG_SIZE];
    CHAR  pMBBuf[MSG_SIZE];
    char  *addr;
    u_short port;
    errno_t et;
    size_t  convLen;
    sockaddr_in peer;
    LPSTR ptr = (LPSTR)recv;

    memset(pWCBuf, 0, MSG_SIZE);
    memset(pMBBuf, 0, MSG_SIZE);
    ptr++;

    // 获取消息长度
    et = memcpy_s(&size, 2, ptr, 2);
    if (et != 0) {
        StringCchPrintf(szTmp, DEFAULT_BUFLEN, _T(MEM_COPY_FAIL), et);
        SCtlTextBufPush(szTmp);
        return FALSE;
    }
    ptr += 2;

    // 获取对端信息
    et = memcpy_s(&peer, metaLen, ptr, metaLen);
    if (et != 0) {
        StringCchPrintf(szTmp, DEFAULT_BUFLEN, _T(MEM_COPY_FAIL), et);
        SCtlTextBufPush(szTmp);
        return FALSE;
    }
    port = ntohs(peer.sin_port);
    addr = inet_ntoa(peer.sin_addr);
    ptr += metaLen;

    // 输出对端信息 "\t<IP>:<PORT> said:"
    et = mbstowcs_s(&convLen, pWCBuf, (size_t)MSG_SIZE, addr, (size_t)MAX_IP_LEN);
    if (et != 0) {
        StringCchPrintf(szTmp, DEFAULT_BUFLEN, _T(FONT_FORMAT_ERROR), et);
        SCtlTextBufPush(szTmp);
        return FALSE;
    }
    if (StringCchPrintf(szTmp, DEFAULT_BUFLEN, _T(PEER_SOCKINFO), pWCBuf, port) != S_OK)    return FALSE;
    SCtlTextBufPush(szTmp);

    // 显示接收的对端消息 "\t<MESS>"
    if (StringCchPrintfA(pMBBuf, size + 1, ptr) != S_OK)    return FALSE;
    memset(pWCBuf, 0, MSG_SIZE);
    et = mbstowcs_s(&convLen, pWCBuf, (size_t)MSG_SIZE, pMBBuf, (size_t)size);
    if (et != 0) {
        StringCchPrintf(szTmp, DEFAULT_BUFLEN, _T(FONT_FORMAT_ERROR), et);
        SCtlTextBufPush(szTmp);
        return FALSE;
    }
    if (StringCchPrintf(szTmp, DEFAULT_BUFLEN, _T(PEER_MESS)) != S_OK)    return FALSE;
    if (StringCchCat(szTmp, DEFAULT_BUFLEN, pWCBuf) != S_OK)    return FALSE;
    SCtlTextBufPush(szTmp);

    return TRUE;
}

BOOL 
UpdateUser(LPCSTR szRecv) {
    LPSTR ptr = (LPSTR)szRecv;
    unsigned short size;
    int     et, inte = sizeof(struct sockaddr);
    u_short port;
    char    *addr;
    char    szBuf[DEFAULT_BUFLEN];
    memset(szBuf, 0, DEFAULT_BUFLEN);
    ptr++;

    // 每次更新总是先重设 combo box
    SendMessageA(hSCtlCombo, (UINT)CB_RESETCONTENT, (WPARAM)0, (LPARAM)0);

    // 获取消息长度
    et = memcpy_s(&size, 2, ptr, 2);
    if (et != 0) {
        StringCchPrintf(szTmp, DEFAULT_BUFLEN, _T(MEM_COPY_FAIL), et);
        SCtlTextBufPush(szTmp);
        return FALSE;
    }
    ptr += 2;

    // 接收用户列表，更新时总是覆盖旧数据
    for (int i = 0, j = 0; i < MAX_CLIENT && j < size; j += inte) {
        et = memcpy_s(&usrList[i], inte, ptr + j, inte);
        if (et != 0) {
            StringCchPrintf(szTmp, DEFAULT_BUFLEN, _T(MEM_COPY_FAIL), et);
            SCtlTextBufPush(szTmp);
            return FALSE;
        }
        port = ntohs(usrList[i].sin_port);
        addr = inet_ntoa(usrList[i].sin_addr);

        // 跳过自己的信息
        if (port == local.port && strcmp(addr, local.addr) == 0)    continue;

        // 显示用户列表
        if (StringCchPrintfA(szBuf, DEFAULT_BUFLEN, "%s:%d", addr, port) != S_OK)    return FALSE;
        SendMessageA(hSCtlCombo, (UINT)CB_ADDSTRING, (WPARAM)0, (LPARAM)szBuf);
        ++i;
    }

    return TRUE;
}

LPSTR 
Packing(int idx, u_short sendLen) {
    errno_t et;
    const int inte = sizeof(struct sockaddr_in);
    int ptr = 1;
    LPSTR buf = (LPSTR)malloc(MSG_SIZE * sizeof(CHAR));
    if (!buf)    return NULL;
    memset(buf, 0, MSG_SIZE);

    // 附加长度
    et = memcpy_s(buf + ptr, sizeof(u_short), &sendLen, sizeof(u_short));
    if (et != 0) {
        StringCchPrintf(szTmp, DEFAULT_BUFLEN, _T(MESS_PACK_FAIL));
        SCtlTextBufPush(szTmp);
        return NULL;
    }
    ptr += 2;

    // 附加对端信息
    et = memcpy_s(buf + ptr, inte, &usrList[idx], inte);
    if (et != 0) {
        StringCchPrintf(szTmp, DEFAULT_BUFLEN, _T(MESS_PACK_FAIL));
        SCtlTextBufPush(szTmp);
        return NULL;
    }
    ptr += inte;

    // 附加消息
    et = memcpy_s(buf + ptr, sendLen, szSend, sendLen);
    if (et != 0) {
        StringCchPrintf(szTmp, DEFAULT_BUFLEN, _T(MESS_PACK_FAIL));
        SCtlTextBufPush(szTmp);
        return NULL;
    }
    ptr += sendLen;

    // 填充至 MSG_SIZE
    //memset(buf + ptr, 0, MSG_SIZE - ptr);

    return buf;
}