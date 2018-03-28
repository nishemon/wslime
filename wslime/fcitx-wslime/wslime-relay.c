#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <unistd.h>
#include <fcntl.h>
#include "wslime.h"

static void
submit(int out_fd, char cmd, int p1, int p2)
{
  static char buff[32];
  sprintf(buff + 1, " %d %d\n", p1, p2);
  buff[0] = cmd;
  size_t toWrite = strlen(buff);
  ssize_t written = 0;
  do {
    written += write(out_fd, buff, toWrite);
  } while (written < toWrite);
}

static wslime_relay_connection g_con;

wslime_relay_connection*
wslime_relay_init()
{
    int cmdpipe[2];
    int respipe[2];
    pipe(cmdpipe);
    pipe(respipe);
    pid_t cpid = fork();
    if (cpid == 0) {
      close(respipe[0]);
      close(cmdpipe[1]);
      close(STDIN_FILENO);
      close(STDOUT_FILENO);
      dup2(cmdpipe[0], STDIN_FILENO);
      dup2(respipe[1], STDOUT_FILENO);
      close(cmdpipe[0]);
      close(respipe[1]);
      char* const argv[] = {
        "\\\\.\\pipe\\wslime",
        "injector",
        NULL
      };
      execve("/mnt/c/Users/takai/source/repos/wslime/x64/Debug/bridge.exe",  argv, argv + 2);
    }
    close(respipe[1]);
    close(cmdpipe[0]);
    g_con.in_fd = respipe[0];
    g_con.out_fd = cmdpipe[1];
    return &g_con;
}

void wslime_relay_wakeup(wslime_relay_connection* con)
{
    submit(con->out_fd, 'O', 1, 0);
}

void wslime_relay_sleep(wslime_relay_connection* con)
{
    submit(con->out_fd, 'O', 0, 0);
}

void wslime_relay_setpoint(wslime_relay_connection* con, int x, int y)
{
    submit(con->out_fd, 'P', x, y);
}

static wslime_composition wc = { 0 };
static char commit_buff[256];
static char str_buff[256];

wslime_composition*
wslime_relay_update(wslime_relay_connection* con, int key, int state)
{
  struct timeval tv;
  fd_set rfds;
  tv.tv_usec = 100000;
  FD_ZERO(&rfds);
  FD_SET(con->in_fd, &rfds);
  wslime_composition* ret = NULL;
  memset(&wc, 0, sizeof(wc));
  for (;;) {
    int sel = select(FD_SETSIZE, &rfds, NULL, NULL, &tv);
    if (sel <= 0) {
      return ret;
    }
    char buff[1024];
    int buffering = read(con->in_fd, buff, sizeof(buff));
    char* rp = buff;
    ret = &wc;
    while (rp < buff + buffering) {
      char* eol = memchr(rp, '\n', buff + buffering - rp);
      if (eol == NULL) {
          eol = buff + buffering;
      }
      if (*rp == 'C') {
        memcpy(commit_buff, rp + 2, eol - rp - 2);
        commit_buff[eol - rp - 2] = '\0';
        wc.commit = commit_buff;
      } else if (*rp == 'E') {
        memcpy(str_buff, rp + 2, eol - rp - 2);
        str_buff[eol - rp - 2] = '\0';
        wc.str = str_buff;
      }
      rp = eol + 1;
    }
    memset(&tv, 0, sizeof(tv));
  }
}

