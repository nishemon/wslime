#include "stdafx.h"
#include <stdio.h>

#include "connector.h"
#include "window.h"
#include "state.h"

static POINT windowOrigin = { 0 };

void wslime_connector_setWindowOrigin(POINT* origin)
{
	windowOrigin = *origin;
}

static void put_composition(HANDLE hRes)
{
	wslime_composition* cmp = wslime_fetch_last_composition();
	BYTE message[1024];
	const size_t clen = (cmp->commit) ? strlen(cmp->commit) : 0;
	const size_t slen = (cmp->str) ? strlen(cmp->str) : 0;
	sprintf_s(message, _countof(message), "C %zu %zu %d %d %d\n", clen, slen, 0, 0, 0);
	if (clen) {
		strcat_s(message, _countof(message), cmp->commit);
		strcat_s(message, _countof(message), "\n");
	}
	if (slen) {
		strcat_s(message, _countof(message), cmp->str);
		strcat_s(message, _countof(message), "\n");
	}
	BYTE* wp = message;
	for (size_t buffering = strlen(message); 0 < buffering; ) {
		DWORD written;
		if (!WriteFile(hRes, wp, (DWORD)buffering, &written, NULL)) {
			return;
		}
		buffering -= written;
		wp += written;
	}
	FlushFileBuffers(hRes);
}


static DWORD parseOneLine(HWND hWnd, const void* buff, size_t len, HANDLE hRes)
{
	const void* end = memchr(buff, '\n', len);
	if (end == NULL) {
		return 0;
	}
	DWORD lineLength = (DWORD)((UINT_PTR)end - (UINT_PTR)buff);
	if (lineLength < 2) {
		return lineLength + 1;
	}
	char* first = ((char*)buff) + 2;
	char* second = memchr(first, ' ', lineLength);
	int f = atoi(first);
	int s = atoi(second);
	switch (((char*)buff)[0]) {
	case 'O':
		if (wslime_setImmStatus(hWnd, !!f)) {
			PostMessage(hWnd, wslime_window_startIMEWindowMessage(), !!f, 0);
		}
		break;
	case 'P':
		wslime_setCursor(f + windowOrigin.x, s + windowOrigin.y);
		break;
	case 'I':
		PostMessage(hWnd, wslime_window_XKeyDownWindowMessage(), f, 0);
		put_composition(hRes);
		break;
	}
	return lineLength + 1;
}


DWORD WINAPI wslime_connector_serverThread(LPVOID lpPrameter)
{
	wslime_ServerIO* io = lpPrameter;
	BYTE buff[256];
	DWORD read;
	DWORD frag = 0;
	static HWND hCurrent = NULL;
	while (ReadFile(io->hCmdConnector, buff + frag, _countof(buff) - frag, &read, NULL) && 0 < read) {
		HWND hActive = GetForegroundWindow();
		if (hActive != hCurrent && wslime_window_isTargetWindow(hActive)) {
			hCurrent = hActive;
		}
		DWORD parsed = 0;
		BYTE* pp = buff;
		DWORD parsable = frag + read;
		do {
			parsed = parseOneLine(hCurrent, pp, parsable, io->hResConnector);
			pp += parsed;
			parsable -= parsed;
		} while (0 < parsed);
		memcpy(buff, pp, parsable);
		frag = parsable;
	}
	return 0;
}
