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
        } else if (fListen)    ServerRun();// 无消息跑服务器
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
        if (!ServerInit(cxChar, cyChar)) {
            SetWindowText(hSCtlLog, _T(NO_AVAILABLE_MEM));
            EnableWindow(hSCtlAddr, FALSE);
            EnableWindow(hSCtlStartButton, FALSE);
        }
        ReleaseDC(hWnd, g_hdc);
        break;
    case WM_PAINT:
        g_hdc = BeginPaint(hWnd, &paintStruct);
        InvalidateRect(hSCtlCap, NULL, FALSE);
        InvalidateRect(hSCtlLog, NULL, FALSE);
        InvalidateRect(hSCtlAddr, NULL, FALSE);
        InvalidateRect(hSCtlPort, NULL, FALSE);
        InvalidateRect(hSCtlStartButton, NULL, FALSE);
        EndPaint(hWnd, &paintStruct);
        break;
    case WM_COMMAND:
        g_hdc = GetDC(hWnd);
        wmId = LOWORD(wParam);
        wmEvent = HIWORD(wParam);
        switch (wmId) {
        case IDC_SERVER_STARTUP_BUTT:   // Start 按钮
            EnableWindow(hSCtlAddr, FALSE);
            EnableWindow(hSCtlPort, FALSE);
            EnableWindow(hSCtlStartButton, FALSE);
            if (!ServerConfig()) {
                EnableWindow(hSCtlAddr, TRUE);
                EnableWindow(hSCtlPort, FALSE);
                EnableWindow(hSCtlStartButton, TRUE);
            }
            break;
        default: return DefWindowProc(hWnd, message, wParam, lParam);
        }
        ReleaseDC(hWnd, g_hdc);
        break;
    case WM_DESTROY:
        free(szSCtlBuf.buf);
        ServerFree();
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
        (HMENU)IDC_SERVER_TEXT_CAPTION,
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
        (HMENU)IDC_SERVER_TEXT_BODY,
        (HINSTANCE)GetModuleHandle(NULL),
        NULL
    );
    // 创建编辑框
    hSCtlAddr = CreateWindow(
        _T("Edit"),
        _T("127.0.0.1"),
        WS_CHILD | WS_BORDER | WS_VISIBLE | ES_AUTOHSCROLL,
        MARGIN, MARGIN + 40 + MARGIN + 250 + MARGIN, 220, 40,
        hWnd,
        (HMENU)IDC_SERVER_ADDR,
        (HINSTANCE)GetModuleHandle(NULL),
        NULL
    );
    // 创建编辑框
    hSCtlPort = CreateWindow(
        _T("Edit"),
        _T("8888"),
        WS_CHILD | WS_BORDER | WS_VISIBLE | ES_AUTOHSCROLL,
        MARGIN + 220 + MARGIN, MARGIN + 40 + MARGIN + 250 + MARGIN, 160, 40,
        hWnd,
        (HMENU)IDC_SERVER_PORT,
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
        (HMENU)IDC_SERVER_STARTUP_BUTT,
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
    mp.clear();
    hListen  = INVALID_SOCKET;
    fListen  = FALSE;

    // 初始化用户 socket 数组
    memset(g_sock, 0, sizeof(SOCKINFO) * MAX_CLIENT);
    memset(usrList, 0, MAX_CLIENT * 16);

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
    int err, iLen = szSCtlBuf.tot, port;
    const int PORT_BIT = 11;
    TCHAR szDefServPort[PORT_BIT];//INT_MAX 最多 10 位
    struct addrinfoW servHint, * result = NULL;
    unsigned long nonblocking = 1;
    int iTime = TIMEOUT;
    LPTSTR szErr = (LPTSTR)malloc(iLen * sizeof(TCHAR));
    if (!szErr)    return FALSE;
    GetWindowText(hSCtlAddr, szDefHost, MAX_IP_LEN);
    // 就算用户输入 111..111 也最多接收 9 个 1，INT_MAX 10 位也可能爆，只允许 9 个
    GetWindowText(hSCtlPort, szDefServPort, PORT_BIT);
    port = _wtoi(szDefServPort);
    if (port < 0 || port > 65535) {
        // 用 int 来接收 short 的端口，才能知道有没有超出 short 的范围
        SCtlTextBufPush(_T(PORT_INP_INCORRECT));
        return FALSE;
    }

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

    // 第四个参数是取指针地址而不是结构体地址
    err = GetAddrInfo(szDefHost, szDefServPort, &servHint, &result);
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
    fListen = TRUE;

    return TRUE;

CLEAN_UP:
    fListen = FALSE;
    if (hListen != INVALID_SOCKET)    closesocket(hListen);
    WSACleanup();
    free(szErr);
    return FALSE;
}

void 
FreeSockInfo(int i) {
    g_sockNum--;
    closesocket(g_sock[i].sock);
    free(g_sock[i].buf);

    for (int j = i; j < g_sockNum; ++j)
        g_sock[j] = g_sock[j + 1];
}

void 
ServerRun() {
    int err, iLen = szSCtlBuf.tot, i, iRes;
    const int intev = sizeof(struct sockaddr);
    uint64_t elem;
    errno_t et;
    unsigned long nonblocking = 1;
    struct sockaddr_in *p;
    struct fd_set readfds;
    LPTSTR szErr = (LPTSTR)malloc(iLen * sizeof(TCHAR));
    LPSTR  szBuf = (LPSTR)malloc(intev * sizeof(CHAR));
    if (!szErr || !szBuf)    return;

    memset(szErr, 0, iLen);
    memset(szBuf, 0, intev);
    // 每次都清空待读集合
    FD_ZERO(&readfds);
    FD_SET(hListen, &readfds);

    // 将整个数组的 socket 都加入集合
    for (int i = 0; i < g_sockNum; ++i)
        FD_SET(g_sock[i].sock, &readfds);

    // 最后一个参数设 0 即可，这样 select() 就是非阻塞的，每次执行该函数都是看一眼就走
    TIMEVAL tv = { 0, 0 };
    int tot = select(0, &readfds, NULL, NULL, &tv);
    if (tot == SOCKET_ERROR) {
        StringCchPrintf(szErr, iLen, _T(SELECT_FAIL), WSAGetLastError());
        SCtlTextBufPush(szErr);
        goto CLEAN_UP;
    }

    // 先检查 hListen 是否有新连接请求
    if (FD_ISSET(hListen, &readfds)) {
        --tot;

        // accept
        struct sockaddr_in user;
        user.sin_family = AF_INET;
        int size = sizeof(struct sockaddr_in);
        SOCKET sockData = accept(hListen, (struct sockaddr *)&user, &size);
        if (sockData == INVALID_SOCKET && WSAGetLastError() != WSAEWOULDBLOCK) {
            StringCchPrintf(szErr, iLen, _T(ACCEPT_FAIL), WSAGetLastError());
            SCtlTextBufPush(szErr);
            return;
        }

        // 将实际用于数据传输的用户 socket 设为非阻塞
        err = ioctlsocket(sockData, FIONBIO, &nonblocking);
        if (err == SOCKET_ERROR) {
            StringCchPrintf(szErr, iLen, _T(USR_IOMODEL_FAIL), WSAGetLastError());
            SCtlTextBufPush(szErr);
            closesocket(sockData);
            return;
        }

        // 打印用户信息（先将 ip 转为宽字符）
        size_t convLen;
        LPTSTR pWCBuf = (LPTSTR)malloc(MAX_IP_LEN * sizeof(TCHAR));
        if (!pWCBuf)    goto CLEAN_UP;
        if (mbstowcs_s(&convLen, pWCBuf, (size_t)MAX_IP_LEN, inet_ntoa(user.sin_addr), (size_t)MAX_IP_LEN - 1) != 0) {
            free(pWCBuf);
            goto CLEAN_UP;
        }
        if (StringCchPrintf(szErr, iLen, _T(ACCEPT_SUCC), pWCBuf, ntohs(user.sin_port)) != S_OK)
            goto CLEAN_UP;
        if (!SCtlTextBufPush(szErr))
            goto CLEAN_UP;
        free(pWCBuf);

        // 添加用户 socket 至全局数组
        g_sock[g_sockNum].uInfo = user;
        g_sock[g_sockNum].sock = sockData;
        g_sock[g_sockNum].buf = (LPSTR)malloc(DEFAULT_BUFLEN * sizeof(CHAR));
        if (!g_sock[g_sockNum].buf)    goto CLEAN_UP;
        memset(g_sock[g_sockNum].buf, 0, DEFAULT_BUFLEN);

        // 更新用户列表（总是覆盖已经离开的用户对应的内存）
        p = &user;// sockaddr_in 结构体转换二进制形式
        for (i = 0; i < intev; ++i)    szBuf[i] = *((char*)p + i);
        et = memcpy_s(usrList + g_sockNum * intev, intev, szBuf, intev);
        if (et != 0)    goto CLEAN_UP;

        // 加入哈希表
        et = memcpy_s(&elem, sizeof(uint64_t), &user, sizeof(uint64_t));
        if (et != 0)    goto CLEAN_UP;
        mp[elem] = sockData;

        ++g_sockNum;

        // 有用户上线，广播用户列表
        if (!SendUserList()) {
            StringCchPrintf(szErr, iLen, _T(USER_LIST_FAIL));
            SCtlTextBufPush(szErr);
            goto CLEAN_UP;
        }
    }

    // 遍历整个全局数组检查每个 socket 是否产生 IO 事件
    // 实际上第一次新加入的数据传输 socket 此处不满足，会在下一次循环才进来 for()
    for (i = 0; tot > 0 && i < g_sockNum; ++i) {
        if (FD_ISSET(g_sock[i].sock, &readfds)) {
            --tot;
            iRes = recv(g_sock[i].sock, g_sock[i].buf, DEFAULT_BUFLEN, 0);
            if (iRes == SOCKET_ERROR) {
                if (WSAGetLastError() != WSAEWOULDBLOCK) {
                    StringCchPrintf(szErr, iLen, _T(MESS_RECV_FAIL), WSAGetLastError());
                    SCtlTextBufPush(szErr);
                    FreeSockInfo(i);
                    continue;
                } else    continue;// 如果是 WSAEWOULDBLOCK 只是数据未到达所以继续遍历即可
            } else if (iRes == 0) {
                // 对端连接关闭
                size_t convLen;
                LPTSTR pWCBuf = (LPTSTR)malloc(MAX_IP_LEN * sizeof(TCHAR));
                if (!pWCBuf)    goto CLEAN_UP;
                if (mbstowcs_s(&convLen, pWCBuf, (size_t)MAX_IP_LEN, inet_ntoa(g_sock[i].uInfo.sin_addr), (size_t)MAX_IP_LEN - 1) != 0) {
                    free(pWCBuf);
                    goto CLEAN_UP;
                }
                if (StringCchPrintf(szErr, iLen, _T(TERMINATION_CONN), pWCBuf, ntohs(g_sock[i].uInfo.sin_port)) != S_OK)
                    goto CLEAN_UP;
                if (!SCtlTextBufPush(szErr))
                    goto CLEAN_UP;
                free(pWCBuf);
                
                // 删除哈希
                et = memcpy_s(&elem, sizeof(uint64_t), &g_sock[i].uInfo, sizeof(uint64_t));
                if (et != 0)    goto CLEAN_UP;
                mp.erase(elem);

                // 释放 sockinfo
                FreeSockInfo(i);

                // 有用户下线，广播列表
                if (!SendUserList()) {
                    StringCchPrintf(szErr, iLen, _T(USER_LIST_FAIL));
                    SCtlTextBufPush(szErr);
                    goto CLEAN_UP;
                }
            } else    TransferMess(i);// 转发聊天消息
        }
    }

CLEAN_UP:
    free(szErr);
    free(szBuf);
}

void 
ServerFree() {
    // 释放所有用户 socket 数组
    for (int i = 0; i < g_sockNum; ++i) {
        closesocket(g_sock[i].sock);
        free(g_sock[i].buf);
    }
    g_sockNum = 0;

    // 释放监听 socket
    if (hListen != INVALID_SOCKET)    closesocket(hListen);
    fListen = FALSE;

    // 释放 Winsock2 dll
    WSACleanup();
}

void 
TransferMess(int idx) {
    // 转发时大部分数据是原封不动的，只需要改变对应的 sockaddr 部分字段
    errno_t et;
    uint64_t elem;
    const int inte = sizeof(struct sockaddr_in);
    sockaddr_in peer;
    LPTSTR szErr = (LPTSTR)malloc(DEFAULT_BUFLEN * sizeof(TCHAR));
    if (!szErr)    return;
    memset(szErr, 0, MSG_SIZE);

    /* 解包 */
    // 解包获取对端信息
    int posPeer = 1 + 2;
    et = memcpy_s(&peer, inte, g_sock[idx].buf + posPeer, inte);
    if (et != 0) {
        free(szErr);
        return;
    }

    /* 封装 */
    // 封装本地信息
    et = memcpy_s(g_sock[idx].buf + posPeer, inte, &g_sock[idx].uInfo, inte);
    if (et != 0) {
        free(szErr);
        return;
    }

    /* 转发 */
    et = memcpy_s(&elem, sizeof(uint64_t), &peer, sizeof(uint64_t));
    if (et != 0) {
        free(szErr);
        return;
    }
    int sendLen = send(mp[elem], g_sock[idx].buf, MSG_SIZE, 0);
    if (sendLen == SOCKET_ERROR && WSAGetLastError() != WSAEWOULDBLOCK) {
        StringCchPrintf(szErr, DEFAULT_BUFLEN, _T(MESS_SEND_FAIL), WSAGetLastError());
        SCtlTextBufPush(szErr);
        free(szErr);
        return;
    }

    free(szErr);
}

BOOL 
SendUserList() {
    const int inte = sizeof(struct sockaddr_in);
    int ptr = 0, err;
    u_short size;
    errno_t et;
    LPTSTR szErr = (LPTSTR)malloc(DEFAULT_BUFLEN * sizeof(TCHAR));
    LPSTR buf = (LPSTR)malloc(MSG_SIZE * sizeof(CHAR));
    if (!buf || !szErr)    return NULL;
    memset(buf, 0, MSG_SIZE);
    memset(szErr, 0, DEFAULT_BUFLEN);

    // 封装包头
    buf[ptr] = 0x7f;
    ptr++;

    // 封装长度
    size = g_sockNum * inte;
    et = memcpy_s(buf + ptr, sizeof(u_short), &size, sizeof(u_short));
    if (et != 0)    return FALSE;
    ptr += 2;

    // 封装用户列表
    for (int i = 0; i < g_sockNum; ++i, ptr += inte) {
        et = memcpy_s(buf + ptr, inte, &g_sock[i].uInfo, inte);
        if (et != 0) {
            StringCchPrintf(szErr, DEFAULT_BUFLEN, _T(USER_LIST_UNPACK));
            SCtlTextBufPush(szErr);
            free(szErr);
            free(buf);
            return NULL;
        }
    }

    // 广播
    for (int i = 0; i < g_sockNum; ++i) {
        err = send(g_sock[i].sock, buf, MSG_SIZE, 0);
        if (err == SOCKET_ERROR && WSAGetLastError() != WSAEWOULDBLOCK)
            return FALSE;
    }

    free(szErr);
    return TRUE;
}