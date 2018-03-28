#include <fcitx/instance.h>
#include <fcitx/context.h>
#include <fcitx/candidate.h>
#include <fcitx/hook.h>
#include <fcitx-config/xdg.h>
//#include <fcitx/module/freedesktop-notify/fcitx-freedesktop-notify.h>
#include <libintl.h>
#include "wslime.h"
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#define _(x) dgettext("fcitx-wslime", (x))

#define DEBUG(fd, msg) write(fd, msg, strlen(msg)); write(fd, "\n", 1)

typedef struct _FcitxWSLime {
    FcitxInstance* owner;
    boolean firstRun;
    int fd;
    boolean init;
    boolean support_client_preedit;
    wslime_connection* con;
} FcitxWSLime;

static void* fcitx_wslime_Create(FcitxInstance* instance);
static void fcitx_wslime_Destroy(void* arg);
static boolean fcitx_wslime_Init(void* arg);
static void fcitx_wslime_Reset(void* arg);
static void fcitx_wslime_ResetHook(void* arg);
static void fcitx_wslime_ReloadConfig(void* arg);
static INPUT_RETURN_VALUE fcitx_wslime_DoInput(void* arg, FcitxKeySym sym, unsigned int state);
static INPUT_RETURN_VALUE fcitx_wslime_DoReleaseInput(void* arg, FcitxKeySym sym, unsigned int state);
static INPUT_RETURN_VALUE fcitx_wslime_DoInputReal(void* arg, FcitxKeySym _sym, unsigned int _state);

FCITX_EXPORT_API
FcitxIMClass ime = {
    fcitx_wslime_Create,
    fcitx_wslime_Destroy
};

FCITX_EXPORT_API
int ABI_VERSION = FCITX_ABI_VERSION;

static void fcitx_wslime_start(FcitxWSLime* wslime, boolean fullcheck) {
    wslime->fd = open("/tmp/fcitx.debug", O_WRONLY | O_CREAT | O_TRUNC);
    wslime->con = wslime_init();
}

static void* fcitx_wslime_Create(FcitxInstance* instance)
{
    FcitxWSLime* wslime = (FcitxWSLime*) fcitx_utils_malloc0(sizeof(FcitxWSLime));
    wslime->owner = instance;
    wslime->firstRun = true;
    fcitx_wslime_start(wslime, false);

    FcitxIMIFace iface;
    memset(&iface, 0, sizeof(FcitxIMIFace));
    iface.Init = fcitx_wslime_Init;
    iface.ResetIM = fcitx_wslime_Reset;
    iface.DoInput = fcitx_wslime_DoInput;
    iface.DoReleaseInput = fcitx_wslime_DoReleaseInput;
    iface.ReloadConfig = fcitx_wslime_ReloadConfig;

    FcitxInstanceRegisterIMv2(
        instance,
        wslime,
        "wslime",
        _("WSLime"),
        "wslime",
        iface,
        10,
        "ja"
    );

    FcitxIMEventHook hook;
    hook.arg = wslime;
    hook.func = fcitx_wslime_ResetHook;
    FcitxInstanceRegisterResetInputHook(instance, hook);
    return wslime;
}

void fcitx_wslime_Destroy(void* arg)
{
    FcitxWSLime* wslime = (FcitxWSLime*) arg;
    free(wslime);
}

boolean fcitx_wslime_Init(void* arg)
{
    FcitxWSLime* wslime = (FcitxWSLime*) arg;
    DEBUG(wslime->fd, "init");
    boolean flag = true;
    FcitxInstanceSetContext(wslime->owner, CONTEXT_IM_KEYBOARD_LAYOUT, "jp");
    FcitxInstanceSetContext(wslime->owner, CONTEXT_DISABLE_AUTO_FIRST_CANDIDATE_HIGHTLIGHT, &flag);
    FcitxInstanceSetContext(wslime->owner, CONTEXT_DISABLE_AUTOENG, &flag);
    FcitxInstanceSetContext(wslime->owner, CONTEXT_DISABLE_QUICKPHRASE, &flag);
    FcitxInstanceCleanInputWindow(wslime->owner);
    return true;
}


void fcitx_wslime_Reset(void* arg)
{
    FcitxWSLime* wslime = (FcitxWSLime*) arg;
    DEBUG(wslime->fd, "reset");
}

void fcitx_wslime_ResetHook(void* arg)
{
    FcitxWSLime* wslime = (FcitxWSLime*) arg;
    FcitxIM* im = FcitxInstanceGetCurrentIM(wslime->owner);
    if (im && strcmp(im->uniqueName, "wslime") == 0) {
        wslime_wakeup(wslime->con);
    } else {
        wslime_sleep(wslime->con);
    }
}

INPUT_RETURN_VALUE fcitx_wslime_DoReleaseInput(void* arg, FcitxKeySym _sym, unsigned int _state)
{
#if 0
    FcitxWSLime *wslime = (FcitxWSLime *)arg;
    FcitxInputState *input = FcitxInstanceGetInputState(wslime->owner);
    uint32_t sym = FcitxInputStateGetKeySym(input);
    uint32_t state = FcitxInputStateGetKeyState(input);
    _state &= (FcitxKeyState_SimpleMask);

/*
    if (_state & (~(FcitxKeyState_Ctrl_Alt_Shift))) {
        return IRV_TO_PROCESS;
    }
*/
    DEBUG(wslime->fd, "doRelInput");
    state &= (FcitxKeyState_SimpleMask);

    return fcitx_wslime_DoInputReal(arg, sym, state | (1 << 30));
#endif
    return IRV_TO_PROCESS;
}

INPUT_RETURN_VALUE fcitx_wslime_DoInput(void* arg, FcitxKeySym _sym, unsigned int _state)
{
    FcitxWSLime *wslime = (FcitxWSLime *)arg;
    FcitxInputState *input = FcitxInstanceGetInputState(wslime->owner);
    uint32_t sym = FcitxInputStateGetKeySym(input);
    uint32_t state = FcitxInputStateGetKeyState(input);
    _state &= (FcitxKeyState_SimpleMask | FcitxKeyState_CapsLock);
    DEBUG(wslime->fd, "doInput");
/*
    if (_state & (~(FcitxKeyState_Ctrl_Alt_Shift | FcitxKeyState_CapsLock))) {
        return IRV_TO_PROCESS;
    }
*/
    state &= (FcitxKeyState_SimpleMask | FcitxKeyState_CapsLock);

    return fcitx_wslime_DoInputReal(arg, sym, state);
}

INPUT_RETURN_VALUE fcitx_wslime_DoInputReal(void* arg, FcitxKeySym sym, unsigned int state)
{
    FcitxWSLime *wslime = (FcitxWSLime *)arg;
    int x, y;
    FcitxInputContext* context = FcitxInstanceGetCurrentIC(wslime->owner);
    FcitxInstanceGetWindowPosition(wslime->owner, context, &x, &y);
    wslime_setpoint(wslime->con, x, y);
char buff[256];
sprintf(buff, "POINT %d, %d", x, y);
DEBUG(wslime->fd, buff);
    wslime->support_client_preedit = context->contextCaps & CAPACITY_PREEDIT;
    wslime_composition* composition = wslime_update(wslime->con, sym, state);
   if (composition == NULL) {
       return IRV_TO_PROCESS;
    }
    FcitxInputState *input = FcitxInstanceGetInputState(wslime->owner);
    FcitxMessages* msgPreedit;
    if (wslime->support_client_preedit) {
        msgPreedit = FcitxInputStateGetClientPreedit(input);
    } else {
        msgPreedit = FcitxInputStateGetPreedit(input);
        FcitxInputStateSetShowCursor(input, true);
        DEBUG(wslime->fd, "NOCLIENT-MODE");
    }
    FcitxMessagesSetMessageCount(msgPreedit, 0);
    FcitxInputStateSetClientCursorPos(input, composition->cursor);
    INPUT_RETURN_VALUE irv = IRV_TO_PROCESS;
    if (composition->commit) {
        FcitxInputContext* context = FcitxInstanceGetCurrentIC(wslime->owner);
        FcitxInstanceCommitString(wslime->owner, context, composition->commit);
        FcitxInstanceCleanInputWindow(wslime->owner);
    }
    if (composition->str) {
        DEBUG(wslime->fd, "preedit");
        DEBUG(wslime->fd, composition->str);
        FcitxInstanceCleanInputWindow(wslime->owner);
        FcitxMessagesAddMessageAtLast(msgPreedit, MSG_INPUT, "%s", composition->str);
        irv |= IRV_FLAG_UPDATE_INPUT_WINDOW;
    } else {
        FcitxInstanceCleanInputWindow(wslime->owner);
        FcitxMessagesAddMessageAtLast(msgPreedit, MSG_INPUT, "");
        irv |= IRV_FLAG_UPDATE_INPUT_WINDOW;

    }

// FCITX_ENTER
    return irv;
}

void fcitx_wslime_ReloadConfig(void* arg)
{
}

