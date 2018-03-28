#pragma once

#include "stdafx.h"

UINT wslime_window_startIMEWindowMessage();
UINT wslime_window_XKeyDownWindowMessage();
BOOL wslime_window_isTargetWindow(HWND hWnd);
LRESULT CALLBACK wslime_window_msgProcHook(int nCode, WPARAM wParam, LPARAM lParam);
