#include "stdafx.h"
#include "injector.h"

#include "state.h"


/*
This state works as Queue.
Windows[KEY_DOWN] -> X[KEY_DOWN] -> 

The simplest solution is, 
IM[KEY_DOWN] invoke to put and get(wait if process) method.
But if 
*/

static POINT cursor = { 0 };
static char g_commit[1024] = "\0";
static char g_preedit[1024] = "\0";
static HWND g_hIMEWnd[64] = { 0 };
static HANDLE getAccessMutex()
{
	static HANDLE hMutex = NULL;
	if (hMutex == NULL) {
		hMutex = CreateMutex(NULL, FALSE, TEXT("wslime"));
	}
	return hMutex;
}


void wslime_assertUpdateIME()
{
	WaitForSingleObject(getAccessMutex(), 1000);
	// TODO: WAIT_TIMEOUT
}
void wslime_deassertUpdateIME()
{
	ReleaseMutex(getAccessMutex());
}

void wslime_commit(const char* commit, size_t size)
{
	strncat_s(g_commit, _countof(g_commit), commit, size);
}

void wslime_update_preedit(const char* preedit, size_t size)
{
	strncpy_s(g_preedit, _countof(g_preedit), preedit, size);
}

wslime_composition* wslime_fetch_last_composition()
{
	static char commit_buff[1024];
	static char preedit_buff[1024];
	static wslime_composition ret = {
		.commit = commit_buff,
		.str = preedit_buff
	};
	const HANDLE hMutex = getAccessMutex();
	WaitForSingleObject(hMutex, 5000);
	// TODO: WAIT_TIMEOUT
	strcpy_s(ret.commit, 1024, g_commit);
	strcpy_s(ret.str, 1024, g_preedit);
	g_commit[0] = '\0';
	g_preedit[0] = '\0';
	ReleaseMutex(hMutex);
	return &ret;
}

BOOL wslime_getImmStatus(HWND hWnd)
{
	for (int i = 0; i < _countof(g_hIMEWnd); i++) {
		if (g_hIMEWnd[i] == hWnd) {
			return TRUE;
		}
		if (g_hIMEWnd[i] == NULL) {
			break;
		}
	}
	return FALSE;
}

BOOL wslime_setImmStatus(HWND hWnd, BOOL status)
{
	int cursor = -1;
	for (int i = 0; i < _countof(g_hIMEWnd); i++) {
		if (g_hIMEWnd[i] == hWnd) {
			if (status) {
				return FALSE;
			}
			cursor = i;
		}
		if (g_hIMEWnd[i] == NULL) {
			if (0 <= cursor) {
				g_hIMEWnd[cursor] = g_hIMEWnd[i - 1];
				g_hIMEWnd[i - 1] = NULL;
			} else if (status) {
				g_hIMEWnd[i] = hWnd;
			} else {
				return FALSE;
			}
			return TRUE;
		}
	}
	// TODO
}

void wslime_setCursor(int x, int y)
{
	cursor.x = x;
	cursor.y = y;
}

void wslime_getCursor(POINT* point)
{
	*point = cursor;
}
