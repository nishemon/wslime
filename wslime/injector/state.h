#pragma once
#include "stdafx.h"
#include "wslime-common.h"

void wslime_assertUpdateIME();
void wslime_deassertUpdateIME();
BOOL wslime_getImmStatus(HWND hWnd);
BOOL wslime_setImmStatus(HWND hWnd, BOOL status);
void wslime_commit(const char* commit, size_t size);
void wslime_update_preedit(const char* commit, size_t size);
void wslime_setCursor(int x, int y);
void wslime_getCursor(POINT* point);
wslime_composition* wslime_fetch_last_composition();
