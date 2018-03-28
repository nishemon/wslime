#pragma once

#ifdef INJECTOR_EXPORTS
#define INJECTOR_API __declspec(dllexport)
#else
#define INJECTOR_API __declspec(dllimport)
#endif

INJECTOR_API BOOL inject(HINSTANCE self, LPTSTR pipePath);
void wslime_injector_init();
HHOOK wslime_injector_getHook();
