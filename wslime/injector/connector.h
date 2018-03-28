#pragma once

#include "stdafx.h"

typedef struct {
	HANDLE hCmdConnector;
	HANDLE hResConnector;
} wslime_ServerIO;

void wslime_connector_setWindowOrigin(POINT* origin);
DWORD WINAPI wslime_connector_serverThread(LPVOID lpPrameter);
