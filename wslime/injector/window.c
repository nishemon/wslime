#include "stdafx.h"
#include "window.h"
#include "injector.h"
#include "state.h"

UINT wslime_window_startIMEWindowMessage()
{
	static UINT wm = WM_NULL;
	if (wm == WM_NULL) {
		wm = RegisterWindowMessage(TEXT("WSLIME_WM_STARTIME"));
	}
	return wm;
}

UINT wslime_window_XKeyDownWindowMessage()
{
	static UINT wm = WM_NULL;
	if (wm == WM_NULL) {
		wm = RegisterWindowMessage(TEXT("WSLIME_WM_XKEYDOWN"));
	}
	return wm;
}

BOOL wslime_window_isTargetWindow(HWND hWnd)
{
	TCHAR cnBuff[256];
	GetClassName(hWnd, cnBuff, _countof(cnBuff));
	// TODO: other case
	return StrCmpI(cnBuff, TEXT("VcXsrv/x X rl")) == 0;
}

static void adjustCandidateWindow(HWND hWnd, HIMC hImc)
{
	RECT wndRect = { 0 };
	GetWindowRect(hWnd, &wndRect);
	POINT cursor = { 0 };
	wslime_getCursor(&cursor);
	CANDIDATEFORM form = {
		.dwStyle = CFS_CANDIDATEPOS,
		.ptCurrentPos.x = max(0, cursor.x - wndRect.left),
		.ptCurrentPos.y = max(0, cursor.y - wndRect.top),
	};
	ImmSetCandidateWindow(hImc, &form);
	LOGFONT dummyFont = { .lfHeight=11 };
#if 0
	LOGFONT dummyFont = { 11, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, SHIFTJIS_CHARSET,
		OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH,
		TEXT("‚l‚r ‚oƒSƒVƒbƒN") };
#endif
	ImmSetCompositionFont(hImc, &dummyFont);
}

static void updateComposition(HIMC hImc, DWORD dwIndex)
{
	BYTE buff[1024];
	TCHAR imebuff[1024];
	if (dwIndex & GCS_RESULTSTR) {
		LONG bytes = ImmGetCompositionString(hImc, GCS_RESULTSTR, NULL, 0);
		if (0 < bytes) {
			ImmGetCompositionString(hImc, GCS_RESULTSTR, (LPVOID)imebuff, bytes);
			int buffering = WideCharToMultiByte(CP_UTF8, 0, (LPCTSTR)imebuff, bytes / 2, NULL, 0, NULL, NULL);
			WideCharToMultiByte(CP_UTF8, 0, (LPCTSTR)imebuff, bytes / 2, buff, buffering, NULL, NULL);
			wslime_commit(buff, buffering);
		}
	}
	if (dwIndex & GCS_COMPSTR) {
		LONG bytes = ImmGetCompositionString(hImc, GCS_COMPSTR, NULL, 0);
		ImmGetCompositionString(hImc, GCS_COMPSTR, (LPVOID)imebuff, bytes);
		int buffering = WideCharToMultiByte(CP_UTF8, 0, (LPCTSTR)imebuff, bytes / 2, NULL, 0, NULL, NULL);
		WideCharToMultiByte(CP_UTF8, 0, (LPCTSTR)imebuff, bytes / 2, buff, buffering, NULL, NULL);
		wslime_update_preedit(buff, buffering);
	}
	int cursor = 0;
	int start = 0;
	int end = 0;
	if (dwIndex & GCS_CURSORPOS) {
	}
}

LRESULT CALLBACK wslime_window_msgProcHook(int nCode, WPARAM wParam, LPARAM lParam)
{
	MSG* pMSG = (MSG*)lParam;
	if (0 <= nCode) {
		HIMC hImc = NULL;
		boolean status;
		boolean passthrough = TRUE;
		switch (pMSG->message) {
		case WM_ACTIVATE:
			if (pMSG->wParam & 0xFFFF) {
				// on active
				status = wslime_getImmStatus(pMSG->hwnd);
				hImc = ImmGetContext(pMSG->hwnd);
				if (status ^ ImmGetOpenStatus(hImc)) {
					ImmSetOpenStatus(hImc, status);
				}
			}
			break;
		case WM_KEYDOWN:
			status = wslime_getImmStatus(pMSG->hwnd);
			hImc = ImmGetContext(pMSG->hwnd);
			if (status && ImmGetOpenStatus(hImc)) {
				wslime_assertUpdateIME();
				TranslateMessage(pMSG);
			}
			break;
			//		case WM_IME_SETCONTEXT:
			//		pMSG->lParam &= ~ISC_SHOWUICOMPOSITIONWINDOW;
			//	break;
		case WM_IME_NOTIFY:
			passthrough = (wParam != IMN_SETCOMPOSITIONWINDOW);
			hImc = ImmGetContext(pMSG->hwnd);
			adjustCandidateWindow(pMSG->hwnd, hImc);
			break;
		case WM_IME_STARTCOMPOSITION:
			passthrough = FALSE;
			adjustCandidateWindow(pMSG->hwnd, hImc);
			break;
		case WM_IME_COMPOSITION:
			passthrough = FALSE;
			hImc = ImmGetContext(pMSG->hwnd);
			updateComposition(hImc, (DWORD)lParam);
			wslime_deassertUpdateIME();
			break;
		default:
			// process original message
			if (pMSG->message == wslime_window_startIMEWindowMessage()) {
				hImc = ImmGetContext(pMSG->hwnd);
				ImmSetOpenStatus(hImc, (BOOL)wParam);
				passthrough = FALSE;
			} else if (pMSG->message == wslime_window_XKeyDownWindowMessage()) {
				passthrough = FALSE;
			}
		}
		if (hImc != NULL) {
			ImmReleaseContext(pMSG->hwnd, hImc);
		}
		if (!passthrough) {
			pMSG->message = WM_NULL;
		}
	}
	return CallNextHookEx(wslime_injector_getHook(pMSG->hwnd), nCode, wParam, lParam);
}
