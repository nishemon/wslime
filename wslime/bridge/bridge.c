// bridge.cpp : アプリケーションのエントリ ポイントを定義します。
//

#include "stdafx.h"

HANDLE g_hStdIn;
HANDLE g_hStdOut;

static BOOL createIO(TCHAR* path, TCHAR* launchTarget);
static void destroyIO();
static DWORD WINAPI copyThread(LPVOID lpParameter);
static BOOL launch(TCHAR* arg, TCHAR* connectorPath);

struct _BridgeCopyRequest {
	HANDLE hRead;
	HANDLE hWrite;
};
typedef struct _BridgeCopyRequest BridgeCopyRequest;
#define BUFFER_LENGTH 4096

static HANDLE g_hWriteConnector = NULL;
static HANDLE g_hReadConnector = NULL;

int _tmain(int argc, TCHAR* argv[])
{
	TCHAR* launch = (2 < argc) ? argv[2] : TEXT("injector");
	createIO(TEXT("\\\\.\\pipe\\wslime"), launch);
//	createIO(argv[1], launch);
	g_hStdIn = GetStdHandle(STD_INPUT_HANDLE);
	g_hStdOut = GetStdHandle(STD_OUTPUT_HANDLE);
	BridgeCopyRequest toPipe = {
		g_hStdIn, g_hWriteConnector
	};
	BridgeCopyRequest fromPipe = {
		g_hReadConnector, g_hStdOut
	};
	HANDLE hFromPipeThread = CreateThread(NULL, 0, copyThread, &fromPipe, 0, NULL);
	copyThread(&toPipe);
	WaitForSingleObject(hFromPipeThread, 10000);
	destroyIO();
	ExitProcess(0);
}


static BOOL createIO(TCHAR* path, TCHAR* launchTarget)
{
	static const TCHAR PIPE_PATH[] = TEXT("\\\\.\\pipe");
	int t = _countof(PIPE_PATH);
	if (_tcsnicmp(path, PIPE_PATH, _countof(PIPE_PATH) - 1) == 0) {
		/*
		We cannot use PIPE_ACCESS_DUPLEX. it may be half duplex.
		*/
		TCHAR pathBuffer[256];
		_sntprintf_s(pathBuffer, _countof(pathBuffer), _TRUNCATE, TEXT("%s.cmd"), path);
		HANDLE hWritePipe = CreateNamedPipe(pathBuffer,
			PIPE_ACCESS_OUTBOUND,
			PIPE_TYPE_BYTE | PIPE_READMODE_BYTE | PIPE_WAIT,
			2, 65536, 65536, 3000, NULL);
		_sntprintf_s(pathBuffer, _countof(pathBuffer), _TRUNCATE, TEXT("%s.res"), path);
		HANDLE hReadPipe = CreateNamedPipe(pathBuffer,
			PIPE_ACCESS_INBOUND,
			PIPE_TYPE_BYTE | PIPE_READMODE_BYTE | PIPE_WAIT,
			2, 65536, 65536, 3000, NULL);
		if (launchTarget) {
			launch(launchTarget, path);
		}
		ConnectNamedPipe(hWritePipe, NULL);
//		ConnectNamedPipe(hReadPipe, NULL);
		g_hWriteConnector = hWritePipe;
		g_hReadConnector = hReadPipe;
		return TRUE;
	}
	g_hWriteConnector = CreateFile(path, GENERIC_WRITE,
		FILE_SHARE_READ | FILE_SHARE_DELETE, NULL,
		OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	g_hReadConnector = CreateFile(path, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	return g_hWriteConnector != INVALID_HANDLE_VALUE && g_hReadConnector != INVALID_HANDLE_VALUE;
}

static void destroyIO()
{
	DisconnectNamedPipe(g_hReadConnector);
	DisconnectNamedPipe(g_hWriteConnector);
	CloseHandle(g_hWriteConnector);
	CloseHandle(g_hReadConnector);
}

typedef BOOL(*INJECTFUNC)(HINSTANCE self, LPTSTR pipePath);

static BOOL launch(TCHAR* arg, TCHAR* connectorPath)
{
	static const TCHAR EXE_SUFFIX[] = TEXT(".exe");
	size_t length = _tcslen(arg);
	if (_tcsicmp(arg + length - 4, EXE_SUFFIX) != 0) {
		HMODULE hMod = LoadLibrary(arg);
		if (hMod != NULL) {
			INJECTFUNC f = (INJECTFUNC)GetProcAddress(hMod, "inject");
			if (f != NULL) {
				(*f)(hMod, connectorPath);
			}
			return TRUE;
		}
	}
	return FALSE;
//	CreateProcess(arg, NULL, NULL, NULL, FALSE, )
}


static DWORD WINAPI copyThread(LPVOID lpParameter)
{
	BridgeCopyRequest* req = lpParameter;
	BYTE buffer[BUFFER_LENGTH];
	DWORD buffering;
	while (ReadFile(req->hRead, buffer, BUFFER_LENGTH, &buffering, NULL) && 0 < buffering) {
		BYTE* wp = buffer;
		while (0 < buffering) {
			DWORD written;
			if (!WriteFile(req->hWrite, wp, buffering, &written, NULL)) {
				DWORD dwError = GetLastError();
				return -1;
			}
			buffering -= written;
			wp += written;
		}
		FlushFileBuffers(req->hWrite);
	}
	return 0;
}
