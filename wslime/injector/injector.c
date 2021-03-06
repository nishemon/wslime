#include "stdafx.h"
#include "injector.h"
#include "connector.h"
#include "window.h"

#pragma comment(linker, "/section:shared,rws")
#pragma data_seg("shared")
// 後で何とかする
static HHOOK g_hHook[10] = {0};
static DWORD g_threadId[10] = { 0 };
static int g_index = 0;
static TCHAR g_strPipePath[256] = { 0 };
static DWORD g_targetProcessId = 0;
#pragma data_seg()

static BOOL CALLBACK enumWindowsProc(HWND hWnd, LPARAM lParam);
static BOOL CALLBACK monitorEnumProc(HMONITOR hMonitor, HDC hDC, LPRECT lprcMonitor, LPARAM dwData);
static int findIndex(DWORD tId);


INJECTOR_API BOOL inject(HINSTANCE self, LPTSTR pipePath)
{
	StrCpy(g_strPipePath, pipePath);
	EnumWindows(enumWindowsProc, (LPARAM)self);
	return TRUE;
}

static BOOL CALLBACK enumWindowsProc(HWND hWnd, LPARAM lParam)
{
	if (!wslime_window_isTargetWindow(hWnd)) {
		return TRUE;
	}
	HINSTANCE self = (HINSTANCE)lParam;
	DWORD tId = GetWindowThreadProcessId(hWnd, &g_targetProcessId);
	if (0 <= findIndex(tId)) {
		return TRUE;
	}
	HHOOK hHook = SetWindowsHookEx(WH_GETMESSAGE, wslime_window_msgProcHook, self, tId);
	g_hHook[g_index] = hHook;
	g_threadId[g_index] = tId;
	g_index++;
	return TRUE;
}

HHOOK wslime_injector_getHook()
{
	return g_hHook[findIndex(0)];
}

static POINT windowsOrigin = { 0 };


void wslime_injector_init()
{
	if (g_targetProcessId != GetCurrentProcessId()) {
		return;
	}
	EnumDisplayMonitors(NULL, NULL, monitorEnumProc, 0);
	wslime_connector_setWindowOrigin(&windowsOrigin);
	TCHAR pathBuffer[256];
	_sntprintf_s(pathBuffer, _countof(pathBuffer), _TRUNCATE, TEXT("%s.cmd"), g_strPipePath);
	HANDLE hCmdConnection = CreateFile(pathBuffer, GENERIC_READ, 0, NULL, OPEN_EXISTING, 0, NULL);

	_sntprintf_s(pathBuffer, _countof(pathBuffer), _TRUNCATE, TEXT("%s.res"), g_strPipePath);
	HANDLE hResConnection = CreateFile(pathBuffer, GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL);

	static wslime_ServerIO io = { 0 };

	io.hCmdConnector = hCmdConnection,
	io.hResConnector = hResConnection,
	CreateThread(NULL, 0, wslime_connector_serverThread, (LPDWORD)&io, 0, NULL);
}

static BOOL CALLBACK monitorEnumProc(HMONITOR hMonitor, HDC hDC, LPRECT lprcMonitor, LPARAM dwData)
{
	windowsOrigin.x = min(windowsOrigin.x, lprcMonitor->left);
	windowsOrigin.y = min(windowsOrigin.y, lprcMonitor->top);
	return TRUE;
}

static int findIndex(DWORD tId)
{
	if (tId == 0) {
		tId = GetCurrentThreadId();
	}
	for (int i = 0; i < _countof(g_threadId); i++) {
		if (g_threadId[i] == tId) {
			return i;
		}
	}
	return -1;
}

