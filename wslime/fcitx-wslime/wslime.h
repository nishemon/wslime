#ifndef WSLIME_H
#define WSLIME_H

typedef struct {
    char* commit;
    char* str;
    int sel_start;
    int sel_end;
    int cursor;
} wslime_composition;

#include "wslime-relay.h"
#define wslime_connection WSLIME_IMPL(connection)
wslime_connection* WSLIME_IMPL(init)();
#define wslime_init WSLIME_IMPL(init)
void WSLIME_IMPL(wakeup)(wslime_connection* con);
#define wslime_wakeup WSLIME_IMPL(wakeup)
void WSLIME_IMPL(sleep)(wslime_connection* con);
#define wslime_sleep WSLIME_IMPL(sleep)
void WSLIME_IMPL(setpoint)(wslime_connection* connect, int x, int y);
#define wslime_setpoint WSLIME_IMPL(setpoint)
void WSLIME_IMPL(inputkey)(wslime_connection* connect, int key, int state);
#define wslime_inputkey WSLIME_IMPL(inputkey)
wslime_composition* WSLIME_IMPL(read)(wslime_connection* connect);
#define wslime_read WSLIME_IMPL(read)
#endif
