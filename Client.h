#pragma once
#ifndef _CLIENT_H_
#define _CLIENT_H_

#include <tchar.h>// 使用 _T()
#include <WinSock2.h>
#include <windows.h>
#pragma comment(lib, "winmm.lib")
#pragma comment(lib, "ws2_32.lib")

#define MARGIN					20

#define CLIENT_TEXT_BODY		1
#define CLIENT_EDIT				2
#define CLIENT_BUTTON			3

#define INIT_SEND_BUTT			"Send"

HDC			g_hdc = NULL;				// 全局设备环境句柄
HWND		hSCtlText;					// 静态文本框
HWND		hSCtlEdit;					// 静态输入框
HWND		hSCtlButton;				// 发送按钮句柄

LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
void PaintInit(HWND);

#endif